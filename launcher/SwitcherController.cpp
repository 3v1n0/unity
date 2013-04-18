/*
 * Copyright (C) 2011-2013 Canonical Ltd
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
#include "SwitcherControllerImpl.h"


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
const unsigned FADE_DURATION = 80;

/**
 * Helper comparison functor for sorting application icons.
 */
bool CompareSwitcherItemsPriority(AbstractLauncherIcon::Ptr const& first,
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

}

namespace switcher
{

Controller::Controller(WindowCreator const& create_window)
  : detail_mode([this] { return detail_mode_; })
  , timeout_length(0)
  , detail_on_timeout(true)
  , detail_timeout_length(500)
  , initial_detail_timeout_length(1500)
  , visible_(false)
  , monitor_(0)
  , show_desktop_disabled_(false)
  , detail_mode_(DetailMode::TAB_NEXT_WINDOW)
  , impl_(new Controller::Impl(this, 20, create_window))
{}


Controller::~Controller()
{}


bool Controller::CanShowSwitcher(const std::vector<AbstractLauncherIcon::Ptr>& results) const
{
  bool empty = (IsShowDesktopDisabled() ? results.empty() : results.size() == 1);
  return (!empty && !WindowManager::Default().IsWallActive());
}

void Controller::Show(ShowMode show,
                      SortMode sort,
                      std::vector<AbstractLauncherIcon::Ptr> results)
{
  impl_->Show(show, sort, results);
}

void Controller::Select(int index)
{
  if (Visible())
    impl_->model_->Select(index);
}

void Controller::SetWorkspace(nux::Geometry geo, int monitor)
{
  monitor_ = monitor;
  impl_->workarea_ = geo;

  if (impl_->view_)
    impl_->view_->monitor = monitor_;
}

void Controller::Hide(bool accept_state)
{
  if (Visible())
  {
    impl_->Hide(accept_state);
  }
}

bool Controller::Visible()
{
  return visible_;
}

void Controller::Next()
{
  impl_->Next();
}

void Controller::Prev()
{
  impl_->Prev();
}

SwitcherView::Ptr Controller::GetView() const
{
  return impl_->GetView();
}

bool Controller::IsDetailViewShown()
{
  return impl_->IsDetailViewShown();
}

void Controller::SetDetail(bool value, unsigned int min_windows)
{
  impl_->SetDetail(value, min_windows);
}

void Controller::InitiateDetail()
{
  impl_->InitiateDetail();
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
  return show_desktop_disabled_;
}

void Controller::SetShowDesktopDisabled(bool disabled)
{
  show_desktop_disabled_ = disabled;
}

int Controller::StartIndex() const
{
  return (IsShowDesktopDisabled() ? 0 : 1);
}

Selection Controller::GetCurrentSelection() const
{
  return impl_->GetCurrentSelection();
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

double Controller::Opacity() const
{
  if (!impl_->view_window_)
    return 0.0f;

  return impl_->view_window_->GetOpacity();
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
  .add("detail_timeout_length", detail_timeout_length())
  .add("visible", visible_)
  .add("monitor", monitor_)
  .add("show_desktop_disabled", show_desktop_disabled_)
  .add("detail_mode", static_cast<int>(detail_mode_));
}


Controller::Impl::Impl(Controller* obj,
                       unsigned int load_timeout,
                       Controller::WindowCreator const& create_window)
  :  construct_timeout_(load_timeout)
  ,  obj_(obj)
  ,  create_window_(create_window)
  ,  main_layout_(nullptr)
  ,  bg_color_(0, 0, 0, 0.5)
  ,  fade_animator_(FADE_DURATION)
{
  ubus_manager_.RegisterInterest(UBUS_BACKGROUND_COLOR_CHANGED, sigc::mem_fun(this, &Controller::Impl::OnBackgroundUpdate));

  if (create_window_ == nullptr)
    create_window_ = []() {
        return nux::ObjectPtr<nux::BaseWindow>(new MockableBaseWindow("Switcher"));
    };

  // TODO We need to get actual timing data to suggest this is necessary.
  //sources_.AddTimeoutSeconds(construct_timeout_, [&] { ConstructWindow(); return false; }, LAZY_TIMEOUT);

  fade_animator_.updated.connect([this] (double opacity) {
    if (view_window_)
    {
      view_window_->SetOpacity(opacity);

      if (!obj_->visible_ && opacity == 0.0f)
        HideWindow();
    }
  });
}

void Controller::Impl::OnBackgroundUpdate(GVariant* data)
{
  gdouble red, green, blue, alpha;
  g_variant_get(data, "(dddd)", &red, &green, &blue, &alpha);
  bg_color_ = nux::Color(red, green, blue, alpha);

  if (view_)
    view_->background_color = bg_color_;
}


void Controller::Impl::Show(ShowMode show, SortMode sort, std::vector<AbstractLauncherIcon::Ptr> results)
{
  if (results.empty() || obj_->visible_)
    return;

  if (sort == SortMode::FOCUS_ORDER)
  {
    std::sort(results.begin(), results.end(), CompareSwitcherItemsPriority);
  }

  model_ = std::make_shared<SwitcherModel>(results);
  obj_->AddChild(model_.get());
  model_->selection_changed.connect(sigc::mem_fun(this, &Controller::Impl::OnModelSelectionChanged));
  model_->detail_selection.changed.connect([this] (bool) { sources_.Remove(DETAIL_TIMEOUT); });
  model_->only_detail_on_viewport = (show == ShowMode::CURRENT_VIEWPORT);

  SelectFirstItem();

  obj_->visible_ = true;
  int real_wait = obj_->timeout_length() - fade_animator_.Duration();

  if (real_wait > 0)
  {
    sources_.AddIdle([&] { ConstructView(); return false; }, VIEW_CONSTRUCT_IDLE);
    sources_.AddTimeout(real_wait, [&] { ShowView(); return false; }, SHOW_TIMEOUT);
  }
  else
  {
    ShowView();
  }

  if (obj_->detail_on_timeout)
  {
    auto cb_func = sigc::mem_fun(this, &Controller::Impl::OnDetailTimer);
    sources_.AddTimeout(obj_->initial_detail_timeout_length, cb_func, DETAIL_TIMEOUT);
  }

  ubus_manager_.SendMessage(UBUS_OVERLAY_CLOSE_REQUEST);
  ubus_manager_.SendMessage(UBUS_SWITCHER_SHOWN,
                            g_variant_new("(bi)", true, obj_->monitor_));
}


bool Controller::Impl::OnDetailTimer()
{
  if (obj_->Visible() && !model_->detail_selection)
  {
    SetDetail(true, 2);
    obj_->detail_mode_ = DetailMode::TAB_NEXT_WINDOW;
  }

  return false;
}

void Controller::Impl::OnModelSelectionChanged(AbstractLauncherIcon::Ptr const& icon)
{
  if (obj_->detail_on_timeout)
  {
    auto cb_func = sigc::mem_fun(this, &Controller::Impl::OnDetailTimer);
    sources_.AddTimeout(obj_->detail_timeout_length, cb_func, DETAIL_TIMEOUT);
  }

  if (icon)
  {
    if (!obj_->Visible())
    {
      ubus_manager_.SendMessage(UBUS_SWITCHER_SHOWN,
                                g_variant_new("(bi)", true, obj_->monitor_));
    }

    ubus_manager_.SendMessage(UBUS_SWITCHER_SELECTION_CHANGED,
                              g_variant_new_string(icon->tooltip_text().c_str()));
  }
}

void Controller::Impl::ShowView()
{
  if (!obj_->Visible())
    return;

  ConstructView();

  ubus_manager_.SendMessage(UBUS_SWITCHER_START, NULL);

  if (view_window_)
  {
    view_->live_background = true;
    view_window_->ShowWindow(true);
    view_window_->PushToFront();

    if (fade_animator_.CurrentState() == nux::animation::Animation::State::Running)
    {
      fade_animator_.Reverse();
    }
    else
    {
      fade_animator_.SetStartValue(0.0f).SetFinishValue(1.0f).Start();
    }
  }
}

void Controller::Impl::ConstructWindow()
{
  sources_.Remove(LAZY_TIMEOUT);

  if (!view_window_)
  {
    main_layout_ = new nux::HLayout(NUX_TRACKER_LOCATION);
    main_layout_->SetVerticalExternalMargin(0);
    main_layout_->SetHorizontalExternalMargin(0);

    view_window_ = create_window_();
    view_window_->SetOpacity(0.0f);
    view_window_->SetLayout(main_layout_);
    view_window_->SetBackgroundColor(nux::color::Transparent);
    view_window_->SetGeometry(workarea_);
  }
}

void Controller::Impl::ConstructView()
{
  if (view_ || !model_)
    return;

  sources_.Remove(VIEW_CONSTRUCT_IDLE);

  view_ = SwitcherView::Ptr(new SwitcherView());
  obj_->AddChild(view_.GetPointer());
  view_->SetModel(model_);
  view_->background_color = bg_color_;
  view_->monitor = obj_->monitor_;

  ConstructWindow();
  main_layout_->AddView(view_.GetPointer(), 1);
  view_window_->SetEnterFocusInputArea(view_.GetPointer());
  view_window_->SetGeometry(workarea_);

  view_built.emit();
}

void Controller::Impl::Hide(bool accept_state)
{
  if (accept_state)
  {
    Selection selection = GetCurrentSelection();
    if (selection.application_)
    {
      Time timestamp = 0;
      selection.application_->Activate(ActionArg(ActionArg::Source::SWITCHER, 0,
                                                 timestamp, selection.window_));
    }
  }

  ubus_manager_.SendMessage(UBUS_SWITCHER_END, g_variant_new_boolean(!accept_state));
  ubus_manager_.SendMessage(UBUS_SWITCHER_SHOWN, g_variant_new("(bi)", false, obj_->monitor_));

  sources_.Remove(VIEW_CONSTRUCT_IDLE);
  sources_.Remove(SHOW_TIMEOUT);
  sources_.Remove(DETAIL_TIMEOUT);

  obj_->visible_ = false;

  if (fade_animator_.CurrentState() == nux::animation::Animation::State::Running)
  {
    fade_animator_.Reverse();
  }
  else
  {
    fade_animator_.SetStartValue(1.0f).SetFinishValue(0.0f).Start();
  }
}

void Controller::Impl::HideWindow()
{
  main_layout_->RemoveChildObject(view_.GetPointer());

  view_window_->SetOpacity(0.0f);
  view_window_->ShowWindow(false);
  view_window_->PushToBack();

  model_.reset();
  view_.Release();
}

void Controller::Impl::Next()
{
  if (!model_)
    return;

  if (model_->detail_selection)
  {
    switch (obj_->detail_mode_)
    {
      case DetailMode::TAB_NEXT_WINDOW:
        if (model_->detail_selection_index < model_->DetailXids().size() - 1)
          model_->NextDetail();
        else
          model_->Next();
        break;
      case DetailMode::TAB_NEXT_TILE:
        model_->Next();
        break;
      case DetailMode::TAB_NEXT_WINDOW_LOOP:
        model_->NextDetail(); //looping automatic
        break;
    }
  }
  else
  {
    model_->Next();
  }
}

void Controller::Impl::Prev()
{
  if (!model_)
    return;

  if (model_->detail_selection)
  {
    switch (obj_->detail_mode_)
    {
      case DetailMode::TAB_NEXT_WINDOW:
        if (model_->detail_selection_index > (unsigned int) 0)
          model_->PrevDetail();
        else
          model_->Prev();
        break;
      case DetailMode::TAB_NEXT_TILE:
        model_->Prev();
        break;
      case DetailMode::TAB_NEXT_WINDOW_LOOP:
        model_->PrevDetail(); //looping automatic
        break;
    }
  }
  else
  {
    model_->Prev();
  }
}

SwitcherView::Ptr Controller::Impl::GetView() const
{
  return view_;
}

bool Controller::Impl::IsDetailViewShown()
{
  return model_ && model_->detail_selection();
}

void Controller::Impl::SetDetail(bool value, unsigned int min_windows)
{
  if (value && model_->Selection()->AllowDetailViewInSwitcher() && model_->DetailXids().size() >= min_windows)
  {
    model_->detail_selection = true;
    obj_->detail_mode_ = DetailMode::TAB_NEXT_WINDOW;
  }
  else
  { 
    model_->detail_selection = false;
  }
}

void Controller::Impl::InitiateDetail(bool animate)
{
  if (!model_)
    return;

  if (!model_->detail_selection)
  {
    view_->animate = animate;

    SetDetail(true);
    obj_->detail_mode_ = DetailMode::TAB_NEXT_TILE;

    if (!view_->animate())
    {
      // As soon as the detail selection is changed we re-enable the animations
      auto conn = std::make_shared<sigc::connection>();
      *conn = model_->detail_selection.changed.connect([this, conn] (bool) {
        if (view_)
          view_->animate = true;
        conn->disconnect();
      });
    }
  }
}

void Controller::Impl::NextDetail()
{
  if (!model_)
    return;

  InitiateDetail(true);
  model_->NextDetail();
}

void Controller::Impl::PrevDetail()
{
  if (!model_)
    return;

  InitiateDetail(true);
  model_->PrevDetail();
}

LayoutWindow::Vector Controller::Impl::ExternalRenderTargets()
{
  if (!view_)
  {
    LayoutWindow::Vector result;
    return result;
  }
  return view_->ExternalTargets();
}

guint Controller::Impl::GetSwitcherInputWindowId() const
{
  return view_window_->GetInputWindowId();
}


Selection Controller::Impl::GetCurrentSelection() const
{
  AbstractLauncherIcon::Ptr application;
  Window window = 0;
  if (model_)
  {
    application = model_->Selection();

    if (application)
    {
      if (model_->detail_selection)
      {
        window = model_->DetailSelectionWindow();
      }
      else if (model_->SelectionIsActive())
      {
        window = model_->DetailXids().front();
      }
    }
  }
  return {application, window};
}


void Controller::Impl::SelectFirstItem()
{
  if (!model_)
    return;

  int first_icon_index = obj_->StartIndex();
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

} // switcher namespace
} // unity namespace
