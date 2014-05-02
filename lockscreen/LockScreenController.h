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
#include <UnityCore/ConnectionManager.h>
#include <UnityCore/GLibSource.h>

#include "LockScreenShieldFactory.h"
#include "ScreenSaverDBusManager.h"
#include "unity-shared/BackgroundEffectHelper.h"
#include "unity-shared/UpstartWrapper.h"

namespace unity
{
namespace lockscreen
{

class Controller : public sigc::trackable
{
public:
  Controller(DBusManager::Ptr const&, session::Manager::Ptr const&,
             UpstartWrapper::Ptr const& upstart_wrapper = std::make_shared<UpstartWrapper>(),
             ShieldFactoryInterface::Ptr const& shield_factory = std::make_shared<ShieldFactory>(),
             bool test_mode = false);

  nux::ROProperty<double> opacity;

  bool IsLocked() const;
  bool HasOpenMenu() const;

private:
  friend class TestLockScreenController;

  void EnsureShields(std::vector<nux::Geometry> const& monitors);
  void EnsureBlankWindow();
  void LockScreen();
  void ShowShields();
  void HideShields();
  void ShowBlankWindow();
  void HideBlankWindow();
  void BlankWindowGrabEnable(bool grab);
  void SimulateActivity();
  void ResetPostLockScreenSaver();

  void OnLockRequested(bool prompt);
  void OnUnlockRequested();
  void OnPresenceStatusChanged(bool idle);
  void OnScreenSaverActivationRequest(bool activate);
  void OnPrimaryShieldMotion(int x, int y);

  std::vector<nux::ObjectPtr<AbstractShield>> shields_;
  nux::ObjectWeakPtr<AbstractShield> primary_shield_;
  nux::ObjectPtr<nux::BaseWindow> blank_window_;

  DBusManager::Ptr dbus_manager_;
  session::Manager::Ptr session_manager_;
  indicator::Indicators::Ptr indicators_;
  UpstartWrapper::Ptr upstart_wrapper_;
  ShieldFactoryInterface::Ptr shield_factory_;

  nux::animation::AnimateValue<double> fade_animator_;
  nux::animation::AnimateValue<double> blank_window_animator_;

  bool test_mode_;
  bool prompt_activation_;
  BlurType old_blur_type_;

  connection::Wrapper uscreen_connection_;
  connection::Wrapper suspend_connection_;
  connection::Wrapper hidden_window_connection_;
  connection::Wrapper motion_connection_;
  connection::Wrapper key_connection_;

  glib::Source::UniquePtr lockscreen_timeout_;
  glib::Source::UniquePtr lockscreen_delay_timeout_;
  glib::Source::UniquePtr screensaver_activation_timeout_;
  glib::Source::UniquePtr screensaver_post_lock_timeout_;
};

}
}

#endif