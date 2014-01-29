// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2013-2014 Canonical Ltd
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
* Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
*/

#include "LockScreenController.h"
#include "unity-shared/AnimationUtils.h"
#include "unity-shared/UScreen.h"
#include "unity-shared/WindowManager.h"

namespace unity
{
namespace lockscreen
{
namespace
{
const unsigned int FADE_DURATION = 200;
}

Controller::Controller(session::Manager::Ptr const& session_manager,
                       ShieldFactoryInterface::Ptr const& shield_factory)
  : session_manager_(session_manager)
  , shield_factory_(shield_factory)
  , fade_animator_(FADE_DURATION)
{
  auto* uscreen = UScreen::GetDefault();
  uscreen->changed.connect(sigc::mem_fun(this, &Controller::OnUScreenChanged));

  session_manager_->lock_requested.connect(sigc::mem_fun(this,&Controller::OnLockRequested));
  session_manager_->unlock_requested.connect(sigc::mem_fun(this, &Controller::OnUnlockRequested));

  fade_animator_.updated.connect([this](double value) {
    std::for_each(shields_.begin(), shields_.end(), [value](nux::ObjectPtr<Shield> const& shield) {
      shield->SetOpacity(value);
    });
  });

  fade_animator_.finished.connect([this]() {
    if (fade_animator_.GetCurrentValue() == 0.0f)
      shields_.clear();
  });
}

void Controller::OnUScreenChanged(int primary, std::vector<nux::Geometry> const& monitors)
{
  if (IsLocked())
  {
    auto* uscreen = UScreen::GetDefault();
    EnsureShields(uscreen->GetMonitorWithMouse(), monitors);
  }
}

void Controller::EnsureShields(int monitor_with_mouse, std::vector<nux::Geometry> const& monitors)
{
  int num_monitors = monitors.size();
  int shields_size = shields_.size();

  for (int i = 0; i < num_monitors; ++i)
  {
    if (i >= shields_size)
      shields_.push_back(shield_factory_->CreateShield(session_manager_, i == monitor_with_mouse));

    shields_[i]->SetGeometry(monitors.at(i));
  }

  shields_.resize(num_monitors);
}

void Controller::OnLockRequested()
{
  if (IsLocked())
    return;

  WindowManager& wm = WindowManager::Default();
  wm.SaveInputFocus();

  auto* uscreen = UScreen::GetDefault();
  EnsureShields(uscreen->GetMonitorWithMouse(), uscreen->GetMonitors());

  std::for_each(shields_.begin(), shields_.end(), [](nux::ObjectPtr<Shield> const& shield) {
    shield->ShowWindow(true);
    shield->SetOpacity(0.0);
  });

  animation::StartOrReverse(fade_animator_, animation::Direction::FORWARD);
}

void Controller::OnUnlockRequested()
{
  if (!IsLocked())
    return;

  std::for_each(shields_.begin(), shields_.end(), [](nux::ObjectPtr<Shield> const& shield) {
    shield->UnGrabPointer();
    shield->UnGrabKeyboard();
  });

  WindowManager& wm = WindowManager::Default();
  wm.RestoreInputFocus();

  animation::StartOrReverse(fade_animator_, animation::Direction::BACKWARD);
}

bool Controller::IsLocked() const
{
  return !shields_.empty();
}

} // lockscreen
} // unity
