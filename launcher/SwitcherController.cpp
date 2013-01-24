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

#include "unity-shared/UBusMessages.h"
#include "unity-shared/WindowManager.h"

#include "SwitcherController.h"

namespace unity
{
using launcher::AbstractLauncherIcon;
using launcher::ActionArg;
using ui::LayoutWindow;

namespace
{
  const std::string LAZY_TIMEOUT = "lazy-timeout";
  const std::string SHOW_TIMEOUT = "show-timeout";
  const std::string DETAIL_TIMEOUT = "detail-timeout";
  const std::string VIEW_CONSTRUCT_IDLE = "view-construct-idle";
}

namespace switcher
{

/* Delegation to impl */
Controller::Controller(WindowCreator const& create_window)
  : detail_on_timeout(true)
  , detail_timeout_length(500)
  , initial_detail_timeout_length(1500)
  , impl_(new ShellController(this, 20, create_window))
{
  AddChild(impl_.get());
}

bool Controller::CanShowSwitcher(const std::vector<AbstractLauncherIcon::Ptr>& results) const
{
  return impl_->CanShowSwitcher(results);
}

void Controller::Show(ShowMode show,
                      SortMode sort,
                      std::vector<AbstractLauncherIcon::Ptr> results)
{
  impl_->Show(show, sort, results);
}

void Controller::Select(int index)
{
  impl_->Select(index);
}

void Controller::SetWorkspace(nux::Geometry geo, int monitor)
{
  impl_->SetWorkspace(geo, monitor);
}

void Controller::Hide(bool accept_state)
{
  impl_->Hide(accept_state);
}

bool Controller::Visible()
{
  return impl_->Visible();
}

void Controller::Next()
{
  impl_->Next();
}

void Controller::Prev()
{
  impl_->Prev();
}

SwitcherView* Controller::GetView()
{
  return impl_->GetView();
}

void Controller::SetDetail(bool value, unsigned int min_windows)
{
  impl_->SetDetail(value, min_windows);
}

void Controller::NextDetail()
{
  impl_->NextDetail();
}

void Controller::PrevDetail()
{
  impl_->PrevDetail();
}

LayoutWindow::Vector Controller::ExternalRenderTargets()
{
  return impl_->ExternalRenderTargets();
}

guint Controller::GetSwitcherInputWindowId() const
{
  return impl_->GetSwitcherInputWindowId();
}

bool Controller::IsShowDesktopDisabled() const
{
  return impl_->IsShowDesktopDisabled();
}

void Controller::SetShowDesktopDisabled(bool disabled)
{
  impl_->SetShowDesktopDisabled(disabled);
}

int Controller::StartIndex() const
{
  return impl_->StartIndex();
}

void Controller::SelectFirstItem()
{
  impl_->SelectFirstItem();
}

sigc::connection Controller::ConnectToViewBuilt(const sigc::slot<void> &f)
{
  return impl_->view_built.connect(f);
}

void Controller::SetDetailOnTimeout(bool timeout)
{
  detail_on_timeout = timeout;
}

std::string
Controller::GetName() const
{
  return "SwitcherController";
}

void
Controller::AddProperties(GVariantBuilder* builder)
{
  unity::variant::BuilderWrapper(builder)
  .add("detail_on_timeout", detail_on_timeout())
  .add("initial_detail_timeout_length", initial_detail_timeout_length())
  .add("detail_timeout_length", detail_timeout_length());
}

ShellController::ShellController(Controller* obj,
                                 unsigned int load_timeout,
                                 Controller::WindowCreator const& create_window)
  :  Impl(obj)
  ,  timeout_length(0)
  ,  construct_timeout_(load_timeout)
  ,  create_window_(create_window)
  ,  main_layout_(nullptr)
  ,  monitor_(0)
  ,  visible_(false)
  ,  show_desktop_disabled_(false)
  ,  bg_color_(0, 0, 0, 0.5)
{
  ubus_manager_.RegisterInterest(UBUS_BACKGROUND_COLOR_CHANGED, sigc::mem_fun(this, &ShellController::OnBackgroundUpdate));

  if (create_window_ == nullptr)
    create_window_ = []() {
        return nux::ObjectPtr<nux::BaseWindow>(new nux::BaseWindow("Switcher"));
    };

  // TODO We need to get actual timing data to suggest this is necessary.
  //sources_.AddTimeoutSeconds(construct_timeout_, [&] { ConstructWindow(); return false; }, LAZY_TIMEOUT);
}

void ShellController::OnBackgroundUpdate(GVariant* data)
{
  gdouble red, green, blue, alpha;
  g_variant_get(data, "(dddd)", &red, &green, &blue, &alpha);
  bg_color_ = nux::Color(red, green, blue, alpha);

  if (view_)
    view_->background_color = bg_color_;
}

bool ShellController::CanShowSwitcher(const std::vector<AbstractLauncherIcon::Ptr>& results) const
{
  bool empty = (show_desktop_disabled_ ? results.empty() : results.size() == 1);

  return (!empty && !WindowManager::Default().IsWallActive());
}

void ShellController::Show(ShowMode show, SortMode sort, std::vector<AbstractLauncherIcon::Ptr> results)
{
  if (results.empty())
    return;

  if (sort == SortMode::FOCUS_ORDER)
  {
    std::sort(results.begin(), results.end(), CompareSwitcherItemsPriority);
  }

  model_.reset(new SwitcherModel(results));
  AddChild(model_.get());
  model_->selection_changed.connect(sigc::mem_fun(this, &ShellController::OnModelSelectionChanged));
  model_->only_detail_on_viewport = (show == ShowMode::CURRENT_VIEWPORT);

  SelectFirstItem();

  // XXX: Workaround for a problem related to Alt+TAB which is needed since the
  //   switcher is set as the active window (LP: #1071298)
  if (model_->Selection()->GetQuirk(AbstractLauncherIcon::Quirk::ACTIVE))
    last_active_selection_ = model_->Selection();

  visible_ = true;

  if (timeout_length > 0)
  {
    sources_.AddIdle([&] { ConstructView(); return false; }, VIEW_CONSTRUCT_IDLE);
    sources_.AddTimeout(timeout_length, [&] { ShowView(); return false; }, SHOW_TIMEOUT);
  }
  else
  {
    ShowView();
  }

  if (obj_->detail_on_timeout)
  {
    auto cb_func = sigc::mem_fun(this, &ShellController::OnDetailTimer);
    sources_.AddTimeout(obj_->initial_detail_timeout_length, cb_func, DETAIL_TIMEOUT);
  }

  ubus_manager_.SendMessage(UBUS_PLACE_VIEW_CLOSE_REQUEST);
  ubus_manager_.SendMessage(UBUS_SWITCHER_SHOWN, g_variant_new("(bi)", true, monitor_));
}

void ShellController::Select(int index)
{
  if (visible_)
    model_->Select(index);
}

bool ShellController::OnDetailTimer()
{
  if (visible_ && !model_->detail_selection)
  {
    SetDetail(true, 2);
    detail_mode_ = TAB_NEXT_WINDOW;
  }

  return false;
}

void ShellController::OnModelSelectionChanged(AbstractLauncherIcon::Ptr const& icon)
{
  if (obj_->detail_on_timeout)
  {
    auto cb_func = sigc::mem_fun(this, &ShellController::OnDetailTimer);
    sources_.AddTimeout(obj_->detail_timeout_length, cb_func, DETAIL_TIMEOUT);
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

void ShellController::ShowView()
{
  if (!visible_)
    return;

  ConstructView();

  ubus_manager_.SendMessage(UBUS_SWITCHER_START, NULL);

  if (view_window_) {
    view_window_->ShowWindow(true);
    view_window_->PushToFront();
    view_window_->SetOpacity(1.0f);
    view_window_->EnableInputWindow(true, "Switcher", true /* take focus */, false);
    view_window_->SetInputFocus();
    view_window_->CaptureMouseDownAnyWhereElse(true);
  }
}

void ShellController::ConstructWindow()
{
  sources_.Remove(LAZY_TIMEOUT);

  if (!view_window_)
  {
    main_layout_ = new nux::HLayout(NUX_TRACKER_LOCATION);
    main_layout_->SetVerticalExternalMargin(0);
    main_layout_->SetHorizontalExternalMargin(0);

    view_window_ = create_window_();
    view_window_->SetLayout(main_layout_);
    view_window_->SetBackgroundColor(nux::Color(0x00000000));
    view_window_->SetGeometry(workarea_);
    view_window_->EnableInputWindow(true, "Switcher", false, false);
    view_window_->InputWindowEnableStruts(false);
  }
}

void ShellController::ConstructView()
{
  if (view_ || !model_)
    return;

  sources_.Remove(VIEW_CONSTRUCT_IDLE);

  view_ = SwitcherView::Ptr(new SwitcherView());
  AddChild(view_.GetPointer());
  view_->SetModel(model_);
  view_->background_color = bg_color_;
  view_->monitor = monitor_;
  view_->SetupBackground();

  ConstructWindow();
  main_layout_->AddView(view_.GetPointer(), 1);
  view_window_->SetEnterFocusInputArea(view_.GetPointer());
  view_window_->SetGeometry(workarea_);
  view_window_->SetOpacity(0.0f);

  view_built.emit();
}

void ShellController::SetWorkspace(nux::Geometry geo, int monitor)
{
  monitor_ = monitor;
  workarea_ = geo;

  if (view_)
    view_->monitor = monitor_;
}

void ShellController::Hide(bool accept_state)
{
  if (!visible_)
    return;

  if (accept_state)
  {
    AbstractLauncherIcon::Ptr const& selection = model_->Selection();
    if (selection)
    {
      if (model_->detail_selection)
      {
        selection->Activate(ActionArg(ActionArg::SWITCHER, 0, model_->DetailSelectionWindow()));
      }
      else
      {
        if (selection == last_active_selection_ &&
            !model_->DetailXids().empty())
        {
          selection->Activate(ActionArg(ActionArg::SWITCHER, 0, model_->DetailXids()[0]));
        }
        else
        {
          selection->Activate(ActionArg(ActionArg::SWITCHER, 0));
        }
      }
    }
  }

  ubus_manager_.SendMessage(UBUS_SWITCHER_END, g_variant_new_boolean(!accept_state));

  sources_.Remove(VIEW_CONSTRUCT_IDLE);
  sources_.Remove(SHOW_TIMEOUT);
  sources_.Remove(DETAIL_TIMEOUT);

  model_.reset();
  visible_ = false;

  if (view_)
    main_layout_->RemoveChildObject(view_.GetPointer());

  if (view_window_)
  {
    view_window_->SetOpacity(0.0f);
    view_window_->ShowWindow(false);
    view_window_->PushToBack();
    view_window_->EnableInputWindow(false);
  }

  ubus_manager_.SendMessage(UBUS_SWITCHER_SHOWN, g_variant_new("(bi)", false, monitor_));

  last_active_selection_ = nullptr;

  view_.Release();
}

bool ShellController::Visible()
{
  return visible_;
}

void ShellController::Next()
{
  if (!model_)
    return;

  if (model_->detail_selection)
  {
    switch (detail_mode_)
    {
      case TAB_NEXT_WINDOW:
        if (model_->detail_selection_index < model_->DetailXids().size() - 1)
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

void ShellController::Prev()
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

SwitcherView* ShellController::GetView()
{
  return view_.GetPointer();
}

void ShellController::SetDetail(bool value, unsigned int min_windows)
{
  if (value && model_->DetailXids().size() >= min_windows)
  {
    model_->detail_selection = true;
    detail_mode_ = TAB_NEXT_WINDOW;
  }
  else
  {
    model_->detail_selection = false;
  }
}

void ShellController::NextDetail()
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

void ShellController::PrevDetail()
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

LayoutWindow::Vector ShellController::ExternalRenderTargets()
{
  if (!view_)
  {
    LayoutWindow::Vector result;
    return result;
  }
  return view_->ExternalTargets();
}

guint ShellController::GetSwitcherInputWindowId() const
{
  return view_window_->GetInputWindowId();
}

bool ShellController::IsShowDesktopDisabled() const
{
  return show_desktop_disabled_;
}

void ShellController::SetShowDesktopDisabled(bool disabled)
{
  show_desktop_disabled_ = disabled;
}

int ShellController::StartIndex() const
{
  return (show_desktop_disabled_ ? 0 : 1);
}

bool ShellController::CompareSwitcherItemsPriority(AbstractLauncherIcon::Ptr const& first,
                                              AbstractLauncherIcon::Ptr const& second)
{
  if (first->GetIconType() == second->GetIconType())
    return first->SwitcherPriority() > second->SwitcherPriority();

  if (first->GetIconType() == AbstractLauncherIcon::IconType::DESKTOP)
    return true;

  if (second->GetIconType() == AbstractLauncherIcon::IconType::DESKTOP)
    return false;

  return first->GetIconType() < second->GetIconType();
}

void ShellController::SelectFirstItem()
{
  if (!model_)
    return;

  int first_icon_index = StartIndex();
  int second_icon_index = first_icon_index + 1;

  AbstractLauncherIcon::Ptr const& first  = model_->at(first_icon_index);
  AbstractLauncherIcon::Ptr const& second = model_->at(second_icon_index);

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

  WindowManager& wm = WindowManager::Default();
  for (auto& window : first->Windows())
  {
    guint32 xid = window->window_id();
    unsigned int num = wm.GetWindowActiveNumber(xid);

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

  for (auto& window : second->Windows())
  {
    guint32 xid = window->window_id();
    second_first = std::max<unsigned long long>(wm.GetWindowActiveNumber(xid), second_first);
  }

  if (first_second > second_first)
    model_->Select(first);
  else
    model_->Select(second);
}

/* Introspection */
std::string
ShellController::GetName() const
{
  return "SwitcherControllerImpl";
}

void
ShellController::AddProperties(GVariantBuilder* builder)
{
  unity::variant::BuilderWrapper(builder)
  .add("timeout_length", timeout_length())
  .add("visible", visible_)
  .add("monitor", monitor_)
  .add("show_desktop_disabled", show_desktop_disabled_)
  .add("detail_mode", detail_mode_);
}

} // switcher namespace
} // unity namespace
