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

#include "unity-shared/AnimationUtils.h"
#include "unity-shared/UBusMessages.h"
#include "unity-shared/WindowManager.h"
#include "unity-shared/IconRenderer.h"
#include "unity-shared/UScreen.h"

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
const int XY_OFFSET = 100;
}

namespace switcher
{

Controller::Controller(WindowCreator const& create_window)
  : detail([this] { return impl_->model_ && impl_->model_->detail_selection(); },
           [this] (bool d) { if (impl_->model_) { impl_->model_->detail_selection = d; } return false; })
  , detail_mode([this] { return detail_mode_; })
  , first_selection_mode(FirstSelectionMode::LAST_ACTIVE_VIEW)
  , show_desktop_disabled(false)
  , mouse_disabled(false)
  , timeout_length(0)
  , detail_on_timeout(true)
  , detail_timeout_length(500)
  , initial_detail_timeout_length(1500)
  , visible_(false)
  , monitor_(0)
  , detail_mode_(DetailMode::TAB_NEXT_WINDOW)
  , impl_(new Controller::Impl(this, 20, create_window))
{}

Controller::~Controller()
{}


bool Controller::CanShowSwitcher(const std::vector<AbstractLauncherIcon::Ptr>& results) const
{
  bool empty = (show_desktop_disabled() ? results.empty() : results.size() == 1);
  return (!empty && !WindowManager::Default().IsWallActive());
}

void Controller::Show(ShowMode show,
                      SortMode sort,
                      std::vector<AbstractLauncherIcon::Ptr> const& results)
{
  auto uscreen = UScreen::GetDefault();
  monitor_     = uscreen->GetMonitorWithMouse();

  impl_->Show(show, sort, results);
}

void Controller::AddIcon(AbstractLauncherIcon::Ptr const& icon)
{
  impl_->AddIcon(icon);
}

void Controller::RemoveIcon(AbstractLauncherIcon::Ptr const& icon)
{
  impl_->RemoveIcon(icon);
}

void Controller::Select(int index)
{
  if (Visible())
    impl_->model_->Select(index);
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

nux::Geometry Controller::GetInputWindowGeometry() const
{
  if (impl_->view_window_)
    return impl_->view_window_->GetGeometry();

  return {0, 0, 0, 0};
}

void Controller::Impl::StartDetailMode()
{
  if (obj_->visible_)
  {
    if (obj_->detail() && HasNextDetailRow())
    {
      NextDetailRow();
    }
    else
    {
      SetDetail(true);
    }
  }
}

void Controller::Impl::StopDetailMode()
{
  if (obj_->visible_)
  {
    if (obj_->detail() && HasPrevDetailRow())
    {
      PrevDetailRow();
    }
    else
    {
      SetDetail(false);
    }
  }
}

void Controller::Impl::CloseSelection()
{
  if (obj_->detail())
  {
    if (model_->detail_selection)
    {
      WindowManager::Default().Close(model_->DetailSelectionWindow());
    }
  }
  else
  {
    // Using model_->Selection()->Close() would be nicer, but it wouldn't take
    // in consideration the workspace related settings
    for (auto window : model_->SelectionWindows())
      WindowManager::Default().Close(window);
  }
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

LayoutWindow::Vector const& Controller::ExternalRenderTargets() const
{
  return impl_->ExternalRenderTargets();
}

int Controller::StartIndex() const
{
  return (show_desktop_disabled() ? 0 : 1);
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
Controller::AddProperties(debug::IntrospectionData& introspection)
{
  introspection
  .add("detail_on_timeout", detail_on_timeout())
  .add("initial_detail_timeout_length", initial_detail_timeout_length())
  .add("detail_timeout_length", detail_timeout_length())
  .add("visible", visible_)
  .add("monitor", monitor_)
  .add("show_desktop_disabled", show_desktop_disabled())
  .add("mouse_disabled", mouse_disabled())
  .add("detail_mode", static_cast<unsigned>(detail_mode_))
  .add("first_selection_mode", static_cast<unsigned>(first_selection_mode()));
}


Controller::Impl::Impl(Controller* obj,
                       unsigned int load_timeout,
                       Controller::WindowCreator const& create_window)
  :  construct_timeout_(load_timeout)
  ,  obj_(obj)
  ,  create_window_(create_window)
  ,  icon_renderer_(std::make_shared<ui::IconRenderer>())
  ,  main_layout_(nullptr)
  ,  fade_animator_(FADE_DURATION)
{
  WindowManager::Default().average_color.changed.connect(sigc::mem_fun(this, &Impl::OnBackgroundUpdate));

  if (create_window_ == nullptr)
    create_window_ = []() {
        return nux::ObjectPtr<nux::BaseWindow>(new MockableBaseWindow("Switcher"));
    };

  // TODO We need to get actual timing data to suggest this is necessary.
  //sources_.AddTimeoutSeconds(construct_timeout_, [this] { ConstructWindow(); return false; }, LAZY_TIMEOUT);

  fade_animator_.updated.connect([this] (double opacity) {
    if (view_window_)
    {
      view_window_->SetOpacity(opacity);

      if (!obj_->visible_ && opacity == 0.0f)
        HideWindow();
    }
  });
}

void Controller::Impl::OnBackgroundUpdate(nux::Color const& new_color)
{
  if (view_)
    view_->background_color = new_color;
}

void Controller::Impl::AddIcon(AbstractLauncherIcon::Ptr const& icon)
{
  if (!obj_->visible_ || !model_)
    return;

  model_->AddIcon(icon);
}

void Controller::Impl::RemoveIcon(AbstractLauncherIcon::Ptr const& icon)
{
  if (!obj_->visible_ || !model_)
    return;

  model_->RemoveIcon(icon);
}

void Controller::Impl::Show(ShowMode show_mode, SortMode sort_mode, std::vector<AbstractLauncherIcon::Ptr> const& results)
{
  if (results.empty() || obj_->visible_)
    return;

  model_ = std::make_shared<SwitcherModel>(results, (sort_mode == SortMode::FOCUS_ORDER));
  model_->only_apps_on_viewport = (show_mode == ShowMode::CURRENT_VIEWPORT);
  model_->selection_changed.connect(sigc::mem_fun(this, &Controller::Impl::OnModelSelectionChanged));
  model_->detail_selection.changed.connect([this] (bool) { sources_.Remove(DETAIL_TIMEOUT); });
  model_->updated.connect([this] { if (!model_->Size()) Hide(false); });

  if (!model_->Size())
  {
    model_.reset();
    return;
  }

  SelectFirstItem();

  obj_->AddChild(model_.get());
  obj_->visible_ = true;

  int real_wait = obj_->timeout_length() - fade_animator_.Duration();

  if (real_wait > 0)
  {
    sources_.AddIdle([this] { ConstructView(); return false; }, VIEW_CONSTRUCT_IDLE);
    sources_.AddTimeout(real_wait, [this] { ShowView(); return false; }, SHOW_TIMEOUT);
  }
  else
  {
    ShowView();
  }

  nux::GetWindowCompositor().SetKeyFocusArea(view_.GetPointer());

  ResetDetailTimer(obj_->initial_detail_timeout_length);

  ubus_manager_.SendMessage(UBUS_OVERLAY_CLOSE_REQUEST);
  ubus_manager_.SendMessage(UBUS_SWITCHER_SHOWN,
                            g_variant_new("(bi)", true, obj_->monitor_));
}

void Controller::Impl::ResetDetailTimer(int timeout_length)
{
  if (obj_->detail_on_timeout)
  {
    auto cb_func = sigc::mem_fun(this, &Controller::Impl::OnDetailTimer);
    sources_.AddTimeout(timeout_length, cb_func, DETAIL_TIMEOUT);
  }
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
  ResetDetailTimer(obj_->detail_timeout_length);

  if (icon)
  {
    if (!obj_->Visible())
    {
      ubus_manager_.SendMessage(UBUS_SWITCHER_SHOWN,
                                g_variant_new("(bi)", true, obj_->monitor_));
    }

    ubus_manager_.SendMessage(UBUS_SWITCHER_SELECTION_CHANGED,
                              glib::Variant(icon->tooltip_text()));
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
    animation::StartOrReverse(fade_animator_, animation::Direction::FORWARD);
  }
}

void Controller::Impl::ConstructWindow()
{
  // sources_.Remove(LAZY_TIMEOUT);

  if (!view_window_)
  {
    main_layout_ = new nux::HLayout(NUX_TRACKER_LOCATION);
    main_layout_->SetVerticalExternalMargin(0);
    main_layout_->SetHorizontalExternalMargin(0);

    view_window_ = create_window_();
    view_window_->SetOpacity(0.0f);
    view_window_->SetLayout(main_layout_);
    view_window_->SetBackgroundColor(nux::color::Transparent);
  }
}

nux::Geometry GetSwitcherViewsGeometry()
{
  auto uscreen     = UScreen::GetDefault();
  int monitor      = uscreen->GetMonitorWithMouse();
  auto monitor_geo = uscreen->GetMonitorGeometry(monitor);

  monitor_geo.Expand(-XY_OFFSET, -XY_OFFSET);

  return monitor_geo;
}

void Controller::Impl::ConstructView()
{
  if (view_ || !model_)
    return;

  sources_.Remove(VIEW_CONSTRUCT_IDLE);

  view_ = SwitcherView::Ptr(new SwitcherView(icon_renderer_));
  view_->SetModel(model_);
  view_->background_color = WindowManager::Default().average_color();
  view_->monitor = obj_->monitor_;
  view_->hide_request.connect(sigc::mem_fun(this, &Controller::Impl::Hide));
  view_->switcher_mouse_up.connect([this] (int icon_index, int button) {
    if (button == 3)
      InitiateDetail(true);
  });

  view_->switcher_mouse_move.connect([this] (int icon_index) {
    if (icon_index >= 0)
      ResetDetailTimer(obj_->detail_timeout_length);
  });

  view_->switcher_next.connect(sigc::mem_fun(this, &Impl::Next));
  view_->switcher_prev.connect(sigc::mem_fun(this, &Impl::Prev));
  view_->switcher_start_detail.connect(sigc::mem_fun(this, &Impl::StartDetailMode));
  view_->switcher_stop_detail.connect(sigc::mem_fun(this, &Impl::StopDetailMode));
  view_->switcher_close_current.connect(sigc::mem_fun(this, &Impl::CloseSelection));
  obj_->AddChild(view_.GetPointer());

  ConstructWindow();
  main_layout_->AddView(view_.GetPointer(), 1);
  view_window_->SetEnterFocusInputArea(view_.GetPointer());
  view_window_->SetGeometry(GetSwitcherViewsGeometry());

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

  ubus_manager_.SendMessage(UBUS_SWITCHER_END, glib::Variant(!accept_state));
  ubus_manager_.SendMessage(UBUS_SWITCHER_SHOWN, g_variant_new("(bi)", false, obj_->monitor_));

  sources_.Remove(VIEW_CONSTRUCT_IDLE);
  sources_.Remove(SHOW_TIMEOUT);
  sources_.Remove(DETAIL_TIMEOUT);

  obj_->visible_ = false;

  animation::StartOrReverse(fade_animator_, animation::Direction::BACKWARD);
}

void Controller::Impl::HideWindow()
{
  if (model_->detail_selection())
    obj_->detail.changed.emit(false);

  main_layout_->RemoveChildObject(view_.GetPointer());

  view_window_->SetOpacity(0.0f);
  view_window_->ShowWindow(false);
  view_window_->PushToBack();

  obj_->RemoveChild(model_.get());
  obj_->RemoveChild(view_.GetPointer());

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

void Controller::Impl::SetDetail(bool value, unsigned int min_windows)
{
  if (value && model_->Selection()->AllowDetailViewInSwitcher() && model_->SelectionWindows().size() >= min_windows)
  {
    model_->detail_selection = true;
    obj_->detail_mode_ = DetailMode::TAB_NEXT_WINDOW;
    obj_->detail.changed.emit(true);
  }
  else
  {
    obj_->detail.changed.emit(false);
    model_->detail_selection = false;
  }
}

void Controller::Impl::InitiateDetail(bool animate)
{
  if (!model_)
    return;

  if (!model_->detail_selection)
  {
    SetDetail(true);

    if (!animate)
      view_->SkipAnimation();
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

void Controller::Impl::NextDetailRow()
{
  if (!model_)
    return;

  model_->NextDetailRow();
}

void Controller::Impl::PrevDetailRow()
{
  if (!model_)
    return;

  model_->PrevDetailRow();
}

bool Controller::Impl::HasNextDetailRow() const
{
  if (!model_)
    return false;

  return model_->HasNextDetailRow();
}

bool Controller::Impl::HasPrevDetailRow() const
{
  if (!model_)
    return false;

  return model_->HasPrevDetailRow();
}

LayoutWindow::Vector const& Controller::Impl::ExternalRenderTargets() const
{
  if (!view_)
  {
    static LayoutWindow::Vector empty_list;
    return empty_list;
  }

  return view_->ExternalTargets();
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
        window = model_->SelectionWindows().front();
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

  if (obj_->first_selection_mode == FirstSelectionMode::LAST_ACTIVE_APP)
  {
    model_->Select(second);
    return;
  }

  uint64_t first_highest = 0;
  uint64_t first_second = 0; // first icons second highest active
  uint64_t second_first = 0; // second icons first highest active

  WindowManager& wm = WindowManager::Default();
  auto const& windows = (model_->only_apps_on_viewport) ? first->WindowsOnViewport() : first->Windows();

  for (auto& window : windows)
  {
    uint64_t num = wm.GetWindowActiveNumber(window->window_id());

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

  second_first = second->SwitcherPriority();

  if (first_second > second_first)
    model_->Select(first);
  else
    model_->Select(second);
}

} // switcher namespace
} // unity namespace
