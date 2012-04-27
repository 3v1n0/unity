/*
 * Copyright (C) 2011 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Jason Smith <jason.smith@canonical.com>
 */

#include <math.h>

#include <Nux/Nux.h>
#include <Nux/HLayout.h>

#include "UBusMessages.h"
#include "WindowManager.h"

#include "SwitcherController.h"

namespace unity
{
using launcher::AbstractLauncherIcon;
using launcher::ActionArg;
using ui::LayoutWindowList;

namespace switcher
{
namespace
{
gboolean OnDetailTimerCb(gpointer data);
}

Controller::Controller(unsigned int load_timeout)
  :  timeout_length(75)
  ,  detail_on_timeout(true)
  ,  detail_timeout_length(500)
  ,  initial_detail_timeout_length(1500)
  ,  construct_timeout_(load_timeout)
  ,  view_window_(nullptr)
  ,  main_layout_(nullptr)
  ,  monitor_(0)
  ,  visible_(false)
  ,  show_timer_(0)
  ,  detail_timer_(0)
  ,  lazy_timer_(0)
  ,  view_idle_timer_(0)
  ,  bg_color_(0, 0, 0, 0.5)
{
  ubus_manager_.RegisterInterest(UBUS_BACKGROUND_COLOR_CHANGED, sigc::mem_fun(this, &Controller::OnBackgroundUpdate));

  /* Construct the view after a prefixed timeout, to improve the startup time */
  lazy_timer_ = g_timeout_add_seconds_full(G_PRIORITY_LOW, construct_timeout_, [] (gpointer data) -> gboolean {
    auto self = static_cast<Controller*>(data);
    self->lazy_timer_ = 0;
    self->ConstructWindow();
    return FALSE;
  }, this, nullptr);
}

Controller::~Controller()
{
  if (view_window_)
    view_window_->UnReference();

  if (lazy_timer_)
    g_source_remove(lazy_timer_);

  if (view_idle_timer_)
    g_source_remove(view_idle_timer_);
}

void Controller::OnBackgroundUpdate(GVariant* data)
{
  gdouble red, green, blue, alpha;
  g_variant_get(data, "(dddd)", &red, &green, &blue, &alpha);
  bg_color_ = nux::Color(red, green, blue, alpha);

  if (view_)
    view_->background_color = bg_color_;
}

void Controller::Show(ShowMode show, SortMode sort, bool reverse,
                      std::vector<AbstractLauncherIcon::Ptr> results)
{
  if (sort == SortMode::FOCUS_ORDER)
  {
    std::sort(results.begin(), results.end(), CompareSwitcherItemsPriority);
  }
  
  model_.reset(new SwitcherModel(results));
  AddChild(model_.get());
  model_->selection_changed.connect(sigc::mem_fun(this, &Controller::OnModelSelectionChanged));
  model_->only_detail_on_viewport = (show == ShowMode::CURRENT_VIEWPORT);

  SelectFirstItem();

  visible_ = true;

  if (timeout_length > 0)
  {
    if (view_idle_timer_)
      g_source_remove(view_idle_timer_);

    view_idle_timer_ = g_idle_add_full(G_PRIORITY_LOW, [] (gpointer data) -> gboolean {
      auto self = static_cast<Controller*>(data);
      self->ConstructView();
      self->view_idle_timer_ = 0;
      return FALSE;
    }, this, NULL);

    if (show_timer_)
      g_source_remove (show_timer_);

    show_timer_ = g_timeout_add(timeout_length, [] (gpointer data) -> gboolean {
      auto self = static_cast<Controller*>(data);
      self->ShowView();
      self->show_timer_ = 0;
      return FALSE;
    }, this);
  }
  else
  {
    ShowView();
  }

  if (detail_on_timeout)
  {
    if (detail_timer_)
      g_source_remove (detail_timer_);
    detail_timer_ = g_timeout_add(initial_detail_timeout_length, OnDetailTimerCb, this);
  }

  ubus_manager_.SendMessage(UBUS_PLACE_VIEW_CLOSE_REQUEST);
  ubus_manager_.SendMessage(UBUS_SWITCHER_SHOWN, g_variant_new("(bi)", true, monitor_));
}

void Controller::Select(int index)
{
  if (visible_)
    model_->Select(index);
}

gboolean Controller::OnDetailTimer()
{
  if (visible_ && !model_->detail_selection)
  {
    SetDetail(true, 2);
    detail_mode_ = TAB_NEXT_WINDOW;
  }

  detail_timer_ = 0;
  return FALSE;
}

void Controller::OnModelSelectionChanged(AbstractLauncherIcon::Ptr icon)
{
  if (detail_on_timeout)
  {
    if (detail_timer_)
      g_source_remove(detail_timer_);

    detail_timer_ = g_timeout_add(detail_timeout_length, OnDetailTimerCb, this);
  }

  if (icon)
  {
    if (!visible_)
    {
      ubus_manager_.SendMessage(UBUS_SWITCHER_SHOWN, g_variant_new("(bi)", true, monitor_));
    }

    ubus_manager_.SendMessage(UBUS_SWITCHER_SELECTION_CHANGED,
                              g_variant_new_string(icon->tooltip_text().c_str()));
  }
}

void Controller::ShowView()
{
  if (!visible_)
    return;
  
  ConstructView();

  if (view_window_)
    view_window_->SetOpacity(1.0f);
}

void Controller::ConstructWindow()
{
  if (lazy_timer_)
  {
    g_source_remove(lazy_timer_);
    lazy_timer_ = 0;
  }

  if (!view_window_)
  {
    main_layout_ = new nux::HLayout(NUX_TRACKER_LOCATION);
    main_layout_->SetVerticalExternalMargin(0);
    main_layout_->SetHorizontalExternalMargin(0);

    view_window_ = new nux::BaseWindow("Switcher");
    view_window_->SinkReference();
    view_window_->SetLayout(main_layout_);
    view_window_->SetBackgroundColor(nux::Color(0x00000000));
    view_window_->SetGeometry(workarea_);
  }
}

void Controller::ConstructView()
{
  if (view_ || !model_)
    return;

  if (view_idle_timer_)
  {
    g_source_remove(view_idle_timer_);
    view_idle_timer_ = 0;
  }

  view_ = SwitcherView::Ptr(new SwitcherView());
  AddChild(view_.GetPointer());
  view_->SetModel(model_);
  view_->background_color = bg_color_;
  view_->monitor = monitor_;
  view_->SetupBackground();

  ConstructWindow();
  main_layout_->AddView(view_.GetPointer(), 1);
  view_window_->SetGeometry(workarea_);
  view_window_->SetOpacity(0.0f);
  view_window_->ShowWindow(true);
}

void Controller::SetWorkspace(nux::Geometry geo, int monitor)
{
  monitor_ = monitor;
  workarea_ = geo;

  if (view_)
    view_->monitor = monitor_;
}

void Controller::Hide(bool accept_state)
{
  if (!visible_)
    return;

  if (accept_state)
  {
    AbstractLauncherIcon::Ptr selection = model_->Selection();
    if (selection)
    {
      if (model_->detail_selection)
      {
        selection->Activate(ActionArg(ActionArg::SWITCHER, 0, model_->DetailSelectionWindow ()));
      }
      else
      {
        if (selection->GetQuirk (AbstractLauncherIcon::QUIRK_ACTIVE) &&
            !model_->DetailXids().empty ())
        {
          selection->Activate(ActionArg (ActionArg::SWITCHER, 0, model_->DetailXids()[0]));
        }
        else
        {
          selection->Activate(ActionArg(ActionArg::SWITCHER, 0));
        }
      }
    }
  }

  if (view_idle_timer_)
  {
    g_source_remove(view_idle_timer_);
    view_idle_timer_ = 0;
  }

  model_.reset();
  visible_ = false;

  if (view_)
    main_layout_->RemoveChildObject(view_.GetPointer());

  if (view_window_)
  {
    view_window_->SetOpacity(0.0f);
    view_window_->ShowWindow(false);
  }

  if (show_timer_)
    g_source_remove(show_timer_);
  show_timer_ = 0;

  if (detail_timer_)
    g_source_remove(detail_timer_);
  detail_timer_ = 0;

  ubus_manager_.SendMessage(UBUS_SWITCHER_SHOWN, g_variant_new("(bi)", false, monitor_));

  view_.Release();
}

bool Controller::Visible()
{
  return visible_;
}

void Controller::Next()
{
  if (!model_)
    return;

  if (model_->detail_selection)
  {
    switch (detail_mode_)
    {
      case TAB_NEXT_WINDOW:
        if (model_->detail_selection_index < model_->DetailXids().size () - 1)
          model_->NextDetail();
        else
          model_->Next();
        break;
      case TAB_NEXT_TILE:
        model_->Next();
        break;
      case TAB_NEXT_WINDOW_LOOP:
        model_->NextDetail(); //looping automatic
        break;
    }
  }
  else
  {
    model_->Next();
  }
}

void Controller::Prev()
{
  if (!model_)
    return;

  if (model_->detail_selection)
  {
    switch (detail_mode_)
    {
      case TAB_NEXT_WINDOW:
        if (model_->detail_selection_index > (unsigned int) 0)
          model_->PrevDetail();
        else
          model_->Prev();
        break;
      case TAB_NEXT_TILE:
        model_->Prev();
        break;
      case TAB_NEXT_WINDOW_LOOP:
        model_->PrevDetail(); //looping automatic
        break;
    }
  }
  else
  {
    model_->Prev();
  }
}

SwitcherView* Controller::GetView()
{
  return view_.GetPointer();
}

void Controller::SetDetail(bool value, unsigned int min_windows)
{
  if (value && model_->DetailXids().size () >= min_windows)
  {
    model_->detail_selection = true;
    detail_mode_ = TAB_NEXT_WINDOW;
  }
  else
  {
    model_->detail_selection = false;
  }
}

void Controller::NextDetail()
{
  if (!model_)
    return;

  if (!model_->detail_selection)
  {
    SetDetail(true);
    detail_mode_ = TAB_NEXT_TILE;
  }
  else
  {
    model_->NextDetail();
  }
}

void Controller::PrevDetail()
{
  if (!model_)
    return;

  if (!model_->detail_selection)
  {
    SetDetail(true);
    detail_mode_ = TAB_NEXT_TILE;
    model_->PrevDetail();
  }
  else
  {
    model_->PrevDetail();
  }
}

LayoutWindowList Controller::ExternalRenderTargets()
{
  if (!view_)
  {
    LayoutWindowList result;
    return result;
  }
  return view_->ExternalTargets();
}

bool Controller::CompareSwitcherItemsPriority(AbstractLauncherIcon::Ptr first,
                                              AbstractLauncherIcon::Ptr second)
{
  if (first->GetIconType() == second->GetIconType())
    return first->SwitcherPriority() > second->SwitcherPriority();
    
  if (first->GetIconType() == AbstractLauncherIcon::IconType::TYPE_DESKTOP)
	return true;
	
  if (second->GetIconType() == AbstractLauncherIcon::IconType::TYPE_DESKTOP)
	return false;
  return first->GetIconType() < second->GetIconType();
}

void Controller::SelectFirstItem()
{
  if (!model_)
    return;

  AbstractLauncherIcon::Ptr first  = model_->at(1);
  AbstractLauncherIcon::Ptr second = model_->at(2);

  if (!first)
  {
    model_->Select(0);
    return;
  }
  else if (!second)
  {
    model_->Select(1);
    return;
  }

  unsigned int first_highest = 0;
  unsigned int first_second = 0; // first icons second highest active
  unsigned int second_first = 0; // second icons first highest active

  for (guint32 xid : first->Windows())
  {
    unsigned int num = WindowManager::Default()->GetWindowActiveNumber(xid);

    if (num > first_highest)
    {
      first_second = first_highest;
      first_highest = num;
    }
    else if (num > first_second)
    {
      first_second = num;
    }
  }

  for (guint32 xid : second->Windows())
  {
    second_first = MAX (WindowManager::Default()->GetWindowActiveNumber(xid), second_first);
  }

  if (first_second > second_first)
    model_->Select (first);
  else
    model_->Select (second);
}

/* Introspection */
std::string
Controller::GetName() const
{
  return "SwitcherController";
}

void
Controller::AddProperties(GVariantBuilder* builder)
{
  unity::variant::BuilderWrapper(builder)
  .add("timeout-length", timeout_length())
  .add("detail-on-timeout", detail_on_timeout())
  .add("initial-detail-timeout-length", initial_detail_timeout_length())
  .add("detail-timeout-length", detail_timeout_length())
  .add("visible", visible_)
  .add("monitor", monitor_)
  .add("detail-mode", detail_mode_);
}

namespace
{

gboolean OnDetailTimerCb(gpointer data)
{
  Controller* controller = static_cast<Controller*>(data);
  return controller->OnDetailTimer();
}

}
}
}
