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

#include "LockScreenBaseShield.h"
#include "LockScreenShieldFactory.h"
#include "LockScreenAcceleratorController.h"
#include "SuspendInhibitorManager.h"
#include "ScreenSaverDBusManager.h"
#include "unity-shared/BackgroundEffectHelper.h"
#include "unity-shared/KeyGrabber.h"
#include "unity-shared/UpstartWrapper.h"

namespace unity
{
namespace lockscreen
{

class AbstractUserPromptView;

class Controller : public sigc::trackable
{
public:
  typedef std::shared_ptr<Controller> Ptr;

  Controller(DBusManager::Ptr const&, session::Manager::Ptr const&, key::Grabber::Ptr const&,
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
  void SetupPrimaryShieldConnections();
  void ActivatePanel();
  void SyncInhibitor();

  void OnLockRequested(bool prompt);
  void OnUnlockRequested();
  void OnPresenceStatusChanged(bool idle);
  void OnScreenSaverActivationRequest(bool activate);
  void OnPrimaryShieldMotion(int x, int y);

  std::vector<nux::ObjectPtr<BaseShield>> shields_;
  nux::ObjectWeakPtr<BaseShield> primary_shield_;
  nux::ObjectWeakPtr<AbstractUserPromptView> prompt_view_;
  nux::ObjectPtr<nux::BaseWindow> blank_window_;

  DBusManager::Ptr dbus_manager_;
  session::Manager::Ptr session_manager_;
  key::Grabber::Ptr key_grabber_;
  indicator::Indicators::Ptr indicators_;
  AcceleratorController::Ptr accelerator_controller_;
  UpstartWrapper::Ptr upstart_wrapper_;
  ShieldFactoryInterface::Ptr shield_factory_;
  SuspendInhibitorManager::Ptr suspend_inhibitor_manager_;

  nux::animation::AnimateValue<double> fade_animator_;
  nux::animation::AnimateValue<double> blank_window_animator_;

  bool test_mode_;
  bool prompt_activation_;
  BlurType old_blur_type_;

  connection::Wrapper uscreen_connection_;
  connection::Wrapper hidden_window_connection_;
  connection::Manager primary_shield_connections_;

  glib::Source::UniquePtr lockscreen_timeout_;
  glib::Source::UniquePtr lockscreen_delay_timeout_;
  glib::Source::UniquePtr screensaver_activation_timeout_;
  glib::Source::UniquePtr screensaver_post_lock_timeout_;
};

}
}

#endif
