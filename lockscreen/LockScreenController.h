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

#ifndef UNITY_LOCKSCREEN_CONTROLLER_H
#define UNITY_LOCKSCREEN_CONTROLLER_H

#include <NuxCore/Animation.h>

#include "LockScreenShield.h"
#include "LockScreenShieldFactory.h"
#include "unity-shared/BackgroundEffectHelper.h"
#include "unity-shared/UpstartWrapper.h"

namespace unity
{
namespace lockscreen
{

class Controller : public sigc::trackable
{
public:
  Controller(session::Manager::Ptr const& session_manager,
             UpstartWrapper::Ptr const& upstart_wrapper = std::make_shared<UpstartWrapper>(),
             ShieldFactoryInterface::Ptr const& shield_factory = std::make_shared<ShieldFactory>(),
             bool test_mode = false);

  bool IsLocked() const;

private:
  friend class TestLockScreenController;

  void EnsureShields(int primary, std::vector<nux::Geometry> const& monitors);
  void LockScreenUsingDisplayManager();
  void LockScreenUsingUnity();
  void ShowShields(bool interactive, bool skip_animation);
  void HideShields(bool skip_animation);

  void OnUScreenChanged(int primary, std::vector<nux::Geometry> const& monitors);
  void OnLockRequested();
  void OnUnlockRequested();

  std::vector<nux::ObjectPtr<Shield>> shields_;
  session::Manager::Ptr session_manager_;
  UpstartWrapper::Ptr upstart_wrapper_;
  ShieldFactoryInterface::Ptr shield_factory_;
  nux::animation::AnimateValue<double> fade_animator_;
  bool test_mode_;
  BlurType old_blur_type_;
};

}
}

#endif