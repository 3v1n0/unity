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
* Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
*/

#include "LockScreenController.h"
#include "unity-shared/UScreen.h"

namespace unity
{
namespace lockscreen
{

Controller::Controller(session::Manager::Ptr const& manager)
  : manager_(manager)
  , locked_(false) // FIXME unity can start in lock state!
{
  auto* uscreen = UScreen::GetDefault();

  uscreen->changed.connect(sigc::mem_fun(this, &Controller::OnUScreenChanged));
  manager_->lock_requested.connect(sigc::mem_fun(this, &Controller::OnLockRequested));
  manager_->unlock_requested.connect(sigc::mem_fun(this, &Controller::OnUnlockRequested));
}

void Controller::OnUScreenChanged(int primary, std::vector<nux::Geometry> const& monitors)
{
  auto* uscreen = UScreen::GetDefault();
  EnsureShields(uscreen->GetMonitorWithMouse(), monitors);
}

void Controller::EnsureShields(int monitor_with_mouse, std::vector<nux::Geometry> const& monitors)
{
  int num_monitors = monitors.size();
  int num_shields = num_monitors;
  int shields_size = shields_.size();
  int last_shield = 0;

  for (int i = 0; i < num_shields; ++i, ++last_shield)
  {
    if (i >= shields_size)
    {
      shields_.push_back(CreateShield(i == monitor_with_mouse));
    }
    else if (!shields_.at(i))
    {
      shields_[i] = CreateShield(i == monitor_with_mouse);
    }

    shields_[i]->SetGeometry(monitors.at(i));
  }

  for (int i = last_shield; i < shields_size; ++i)
  {
  }

  shields_.resize(num_shields);
}

nux::ObjectPtr<Shield> Controller::CreateShield(bool is_monitor_with_mouse)
{
  nux::ObjectPtr<Shield> shield(new Shield(is_monitor_with_mouse));
  return shield;
}

void Controller::OnLockRequested()
{
  if (locked_)
    return;

  auto* uscreen = UScreen::GetDefault();
  EnsureShields(uscreen->GetMonitorWithMouse(), uscreen->GetMonitors());

  std::for_each(shields_.begin(), shields_.end(), [](nux::ObjectPtr<Shield> const& shield) {
    shield->SetOpacity(0.5f);
    shield->ShowWindow(true);
  });

  locked_ = true;
}

void Controller::OnUnlockRequested()
{
  if (!locked_)
    return;

  std::for_each(shields_.begin(), shields_.end(), [](nux::ObjectPtr<Shield> const& shield) {
    shield->SetOpacity(0.0f);
    shield->ShowWindow(false);
  });

  locked_ = false;
}



}
}