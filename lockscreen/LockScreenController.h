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

#ifndef UNITY_LOCKSCREEN_CONTROLLER_H
#define UNITY_LOCKSCREEN_CONTROLLER_H

#include <memory>
#include <sigc++/trackable.h>

#include "LockScreenShield.h"
#include <UnityCore/SessionManager.h>

namespace unity
{
namespace lockscreen
{

class Controller : public sigc::trackable
{
public:
  Controller(session::Manager::Ptr const& manager);

private:
  void EnsureShields(int monitor_with_mouse, std::vector<nux::Geometry> const& monitors);
  nux::ObjectPtr<Shield> CreateShield(bool is_monitor_with_mouse);

  void OnUScreenChanged(int primary, std::vector<nux::Geometry> const& monitors);
  void OnLockRequested();
  void OnUnlockRequested();

  session::Manager::Ptr const& manager_;
  std::vector<nux::ObjectPtr<Shield>> shields_;
  bool locked_;
};

}
}

#endif