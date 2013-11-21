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

#include "unity-shared/AnimationUtils.h"
#include "unity-shared/UScreen.h"
#include "unity-shared/UBusMessages.h"
#include "unity-shared/UBusWrapper.h"
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

  WindowManager::Default().average_color.changed.connect(sigc::mem_fun(this, &Controller::OnBackgroundUpdate));

  fade_animator_.updated.connect([this] (double opacity) { view_window_->SetOpacity(opacity); });
  fade_animator_.finished.connect([this] {
    if (animation::GetDirection(fade_animator_) == animation::Direction::BACKWARD)
      CloseWindow();
  });
}

void Controller::OnBackgroundUpdate(nux::Color const& new_color)
{
  if (view_)
    view_->background_color = new_color;
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

  UBusManager().SendMessage(UBUS_OVERLAY_CLOSE_REQUEST);
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
  animation::StartOrReverse(fade_animator_, animation::Direction::FORWARD);
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
  view_ = View::Ptr(new View(manager_));
  view_->background_color = WindowManager::Default().average_color();
  debug::Introspectable::AddChild(view_.GetPointer());

  auto layout = new nux::HLayout(NUX_TRACKER_LOCATION);
  layout->SetVerticalExternalMargin(0);
  layout->SetHorizontalExternalMargin(0);
  layout->AddView(view_.GetPointer());

  view_window_ = new nux::BaseWindow("SessionManager");
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
  animation::StartOrReverse(fade_animator_, animation::Direction::BACKWARD);
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

void Controller::AddProperties(debug::IntrospectionData& introspection)
{
  introspection
    .add("visible", Visible());
}

} // namespace session
} // namespace unity
