// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2013 Canonical Ltd
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
* Authored by: Marco Trevisan (Treviño) <marco@ubuntu.com>
*/

#include "SessionController.h"

#include "unity-shared/UBusMessages.h"
#include "unity-shared/UScreen.h"
#include "unity-shared/WindowManager.h"

namespace na = nux::animation;

namespace unity
{
namespace session
{
namespace
{
const unsigned int FADE_DURATION = 100;
}

Controller::Controller(session::Manager::Ptr const& manager)
  : manager_(manager)
  , bg_color_(0.0, 0.0, 0.0, 0.5)
  , fade_animator_(FADE_DURATION)
{
  manager_->reboot_requested.connect([this] (bool inhibitors) {
    Show(View::Mode::SHUTDOWN, inhibitors);
  });

  manager_->shutdown_requested.connect([this] (bool inhibitors) {
    Show(View::Mode::FULL, inhibitors);
  });

  manager_->logout_requested.connect([this] (bool inhibitors) {
    Show(View::Mode::LOGOUT, inhibitors);
  });

  manager_->cancel_requested.connect(sigc::mem_fun(this, &Controller::Hide));

  ubus_manager_.RegisterInterest(UBUS_BACKGROUND_COLOR_CHANGED,
                                 sigc::mem_fun(this, &Controller::OnBackgroundUpdate));

  ubus_manager_.SendMessage(UBUS_BACKGROUND_REQUEST_COLOUR_EMIT);

  fade_animator_.updated.connect([this] (double opacity) {
    if (!view_window_)
      return;

    view_window_->SetOpacity(opacity);

    if (opacity == 0.0f && fade_animator_.GetFinishValue() == 0.0f)
      CloseWindow();
  });
}

void Controller::OnBackgroundUpdate(GVariant* data)
{
  gdouble red, green, blue, alpha;
  g_variant_get(data, "(dddd)", &red, &green, &blue, &alpha);
  bg_color_ = nux::Color(red, green, blue, alpha);

  if (view_)
    view_->background_color = bg_color_;
}

void Controller::Show(View::Mode mode)
{
  Show(mode, false);
}

void Controller::Show(View::Mode mode, bool inhibitors)
{
  EnsureView();

  if (Visible() && mode == view_->mode())
    return;

  ubus_manager_.SendMessage(UBUS_OVERLAY_CLOSE_REQUEST);
  WindowManager::Default().SaveInputFocus();

  if (nux::GetWindowThread()->IsEmbeddedWindow())
  {
    view_window_->EnableInputWindow(true, view_window_->GetWindowName().c_str(), true, false);
    view_window_->GrabPointer();
    view_window_->GrabKeyboard();
  }

  view_->mode = mode;
  view_->have_inhibitors = inhibitors;
  view_->live_background = true;

  view_window_->ShowWindow(true);
  view_window_->PushToFront();
  view_window_->SetInputFocus();
  nux::GetWindowCompositor().SetKeyFocusArea(view_->key_focus_area());

  if (fade_animator_.CurrentState() == na::Animation::State::Running)
  {
    if (fade_animator_.GetFinishValue() == 0.0f)
      fade_animator_.Reverse();
  }
  else
  {
    fade_animator_.SetStartValue(0.0f).SetFinishValue(1.0f).Start();
  }
}

nux::Point Controller::GetOffsetPerMonitor(int monitor)
{
  EnsureView();

  auto const& view_geo = view_->GetGeometry();
  auto const& monitor_geo = UScreen::GetDefault()->GetMonitorGeometry(monitor);

  nux::Point offset(adjustment_.x + monitor_geo.x, adjustment_.y + monitor_geo.y);
  offset.x += (monitor_geo.width - view_geo.width - adjustment_.x) / 2;
  offset.y += (monitor_geo.height - view_geo.height - adjustment_.y) / 2;

  return offset;
}

void Controller::ConstructView()
{
  view_window_ = new nux::BaseWindow("SessionManager");
  view_ = View::Ptr(new View(view_window_.GetPointer(), manager_));
  view_->background_color = bg_color_;
  debug::Introspectable::AddChild(view_.GetPointer());

  auto layout = new nux::HLayout(NUX_TRACKER_LOCATION);
  layout->SetVerticalExternalMargin(0);
  layout->SetHorizontalExternalMargin(0);
  layout->AddView(view_.GetPointer());

  view_window_->SetLayout(layout);
  view_window_->SetBackgroundColor(nux::color::Transparent);
  view_window_->SetWindowSizeMatchLayout(true);
  view_window_->ShowWindow(false);
  view_window_->SetOpacity(0.0f);
  view_window_->SetEnterFocusInputArea(view_.GetPointer());

  view_window_->mouse_down_outside_pointer_grab_area.connect([this] (int, int, unsigned long, unsigned long) {
    CancelAndHide();
  });

  view_->request_hide.connect(sigc::mem_fun(this, &Controller::Hide));
  view_->request_close.connect(sigc::mem_fun(this, &Controller::CancelAndHide));

  if (nux::GetWindowThread()->IsEmbeddedWindow())
  {
    view_->size_changed.connect([this] (nux::Area*, int, int) {
      int monitor = UScreen::GetDefault()->GetMonitorWithMouse();
      auto const& offset = GetOffsetPerMonitor(monitor);
      view_window_->SetXY(offset.x, offset.y);
    });
  }
  else
  {
    view_window_->SetXY(0, 0);
  }
}

void Controller::EnsureView()
{
  if (!view_window_)
    ConstructView();
}

void Controller::CancelAndHide()
{
  manager_->CancelAction();
  Hide();
}

void Controller::Hide()
{
  if (fade_animator_.CurrentState() == na::Animation::State::Running)
  {
    if (fade_animator_.GetFinishValue() == 1.0f)
      fade_animator_.Reverse();
  }
  else
  {
    fade_animator_.SetStartValue(1.0f).SetFinishValue(0.0f).Start();
  }
}

void Controller::CloseWindow()
{
  view_window_->PushToBack();
  view_window_->ShowWindow(false);
  view_window_->UnGrabPointer();
  view_window_->UnGrabKeyboard();
  view_window_->EnableInputWindow(false);
  view_->live_background = false;

  nux::GetWindowCompositor().SetKeyFocusArea(nullptr);
  WindowManager::Default().RestoreInputFocus();
}

bool Controller::Visible() const
{
  return (view_window_ && view_window_->IsVisible());
}

//
// Introspection
//
std::string Controller::GetName() const
{
  return "SessionController";
}

void Controller::AddProperties(GVariantBuilder* builder)
{
  variant::BuilderWrapper(builder)
    .add("visible", Visible());
}

} // namespace session
} // namespace unity
