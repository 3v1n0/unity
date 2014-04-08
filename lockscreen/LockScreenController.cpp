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

#include <UnityCore/DBusIndicators.h>
#include <NuxCore/Logger.h>

#include "LockScreenShield.h"
#include "LockScreenSettings.h"
#include "unity-shared/AnimationUtils.h"
#include "unity-shared/UScreen.h"
#include "unity-shared/WindowManager.h"

namespace unity
{
namespace lockscreen
{
namespace
{
const unsigned int IDLE_FADE_DURATION = 10000;
const unsigned int LOCK_FADE_DURATION = 400;
}

namespace testing
{
const std::string DBUS_NAME = "com.canonical.Unity.Test.DisplayManager";
}

DECLARE_LOGGER(logger, "unity.locksreen");

Controller::Controller(DBusManager::Ptr const& dbus_manager,
                       session::Manager::Ptr const& session_manager,
                       UpstartWrapper::Ptr const& upstart_wrapper,
                       ShieldFactoryInterface::Ptr const& shield_factory,
                       bool test_mode)
  : session_manager_(session_manager)
  , upstart_wrapper_(upstart_wrapper)
  , shield_factory_(shield_factory)
  , fade_animator_(LOCK_FADE_DURATION)
  , fade_windows_animator_(IDLE_FADE_DURATION)
  , test_mode_(test_mode)
{
  auto* uscreen = UScreen::GetDefault();
  uscreen_connection_ = uscreen->changed.connect([this] (int, std::vector<nux::Geometry> const& monitors) {
    EnsureShields(monitors);
    EnsureFadeWindows(monitors);
  });

  uscreen_connection_->block();

  suspend_connection_ = uscreen->suspending.connect([this] {
    if (Settings::Instance().lock_on_suspend())
      session_manager_->lock_requested.emit();
  });

  session_manager_->lock_requested.connect(sigc::mem_fun(this, &Controller::OnLockRequested));
  session_manager_->unlock_requested.connect(sigc::mem_fun(this, &Controller::OnUnlockRequested));

  session_manager_->presence_status_changed.connect([this](bool is_idle) {
    if (is_idle)
    {
      EnsureFadeWindows(UScreen::GetDefault()->GetMonitors());
      animation::StartOrReverse(fade_windows_animator_, animation::Direction::FORWARD);
    }
    else
    {
      std::for_each(fade_windows_.begin(), fade_windows_.end(), [](nux::ObjectPtr<nux::BaseWindow> const& window) {
        window->ShowWindow(false);
      });

      fade_windows_.clear();
      animation::SetValue(fade_windows_animator_, animation::Direction::BACKWARD);
    }
  });

  fade_animator_.updated.connect([this](double value) {
    std::for_each(shields_.begin(), shields_.end(), [value](nux::ObjectPtr<Shield> const& shield) {
      shield->SetOpacity(value);
    });
  });

  fade_animator_.finished.connect([this] {
    if (animation::GetDirection(fade_animator_) == animation::Direction::BACKWARD)
    {
      motion_connection_->disconnect();
      uscreen_connection_->block();
      session_manager_->unlocked.emit();

      std::for_each(shields_.begin(), shields_.end(), [](nux::ObjectPtr<Shield> const& shield) {
        shield->RemoveLayout();
      });

      shields_.clear();

      upstart_wrapper_->Emit("desktop-unlock");
      indicators_.reset();
    }
  });

  fade_windows_animator_.updated.connect([this](double value) {
    std::for_each(fade_windows_.begin(), fade_windows_.end(), [value](nux::ObjectPtr<nux::BaseWindow> const& window) {
      window->SetOpacity(value);
    });
  });

  fade_windows_animator_.finished.connect([this, dbus_manager]() {
    if (fade_windows_animator_.GetCurrentValue() == 1.0)
    {
      dbus_manager->SetActive(true);

      int lock_delay = Settings::Instance().lock_delay() * 1000;

      lockscreen_delay_timeout_.reset(new glib::Timeout(lock_delay, [this](){
        OnLockRequested();
        return false;
      }));

      std::for_each(shields_.begin(), shields_.end(), [](nux::ObjectPtr<Shield> const& shield) {
        shield->UnGrabPointer();
        shield->UnGrabKeyboard();
      });

      std::for_each(fade_windows_.begin(), fade_windows_.end(), [this](nux::ObjectPtr<nux::BaseWindow> const& window) {
          window->EnableInputWindow(true);
          window->GrabPointer();
          window->GrabKeyboard();

          window->mouse_move.connect([this](int, int, int dx, int dy, unsigned long, unsigned long) {
            if (!dx && !dy)
              return;

            session_manager_->presence_status_changed.emit(false);
          });
      });
    }
    else
    {
      dbus_manager->SetActive(false);
      lockscreen_delay_timeout_.reset();

      std::for_each(fade_windows_.begin(), fade_windows_.end(), [this](nux::ObjectPtr<nux::BaseWindow> const& window) {
        window->UnGrabPointer();
        window->UnGrabKeyboard();
      });

      std::for_each(shields_.begin(), shields_.end(), [](nux::ObjectPtr<Shield> const& shield) {
        if (!shield->primary())
          return;

        shield->GrabPointer();
        shield->GrabKeyboard();
      });

    }
  });
}

void Controller::OnPrimaryShieldMotion(int x, int y)
{
  if (!primary_shield_->GetAbsoluteGeometry().IsPointInside(x, y))
  {
    for (auto const& shield : shields_)
    {
      if (!shield->GetAbsoluteGeometry().IsPointInside(x, y))
        continue;

      primary_shield_->primary = false;
      primary_shield_ = shield;
      shield->primary = true;
      auto move_cb = sigc::mem_fun(this, &Controller::OnPrimaryShieldMotion);
      motion_connection_ = shield->grab_motion.connect(move_cb);
      break;
    }
  }
}

void Controller::EnsureShields(std::vector<nux::Geometry> const& monitors)
{
  int num_monitors = monitors.size();
  int shields_size = shields_.size();
  int primary = UScreen::GetDefault()->GetMonitorWithMouse();

  shields_.resize(num_monitors);

  for (int i = 0; i < num_monitors; ++i)
  {
    auto& shield = shields_[i];
    bool is_new = false;

    if (i >= shields_size)
    {
      shield = shield_factory_->CreateShield(session_manager_, indicators_, i, i == primary);
      is_new = true;
    }

    shield->SetGeometry(monitors[i]);
    shield->SetMinMaxSize(monitors[i].width, monitors[i].height);
    shield->primary = (i == primary);
    shield->monitor = i;

    if (is_new && fade_animator_.GetCurrentValue() > 0)
    {
      shield->SetOpacity(fade_animator_.GetCurrentValue());
      shield->ShowWindow(true);
    }
  }

  primary_shield_ = shields_[primary];
  primary_shield_->primary = true;
  auto move_cb = sigc::mem_fun(this, &Controller::OnPrimaryShieldMotion);
  motion_connection_ = primary_shield_->grab_motion.connect(move_cb);
}

void Controller::EnsureFadeWindows(std::vector<nux::Geometry> const& monitors)
{
  int num_monitors = monitors.size();
  int windows_size = fade_windows_.size();

  fade_windows_.resize(num_monitors);

  for (int i = 0; i < num_monitors; ++i)
  {
    auto& window = fade_windows_[i];
    bool is_new = false;

    if (i >= windows_size)
    {
      window = new nux::BaseWindow();
      window->ShowWindow(true);
      window->PushToFront();
      window->SetBackgroundLayer(new nux::ColorLayer(nux::color::Black, true));
      is_new = true;
    }

    window->SetGeometry(monitors[i]);
    window->SetMinMaxSize(monitors[i].width, monitors[i].height);

    if (is_new && fade_windows_animator_.GetCurrentValue() > 0)
    {
      window->SetOpacity(fade_windows_animator_.GetCurrentValue());
      window->ShowWindow(true);
      window->PushToFront();
    }
  }
}

void Controller::OnLockRequested()
{
  if (!Settings::Instance().lock_enabled())
  {
    LOG_DEBUG(logger) << "Failed to lock screen: lock is not enabled.";
    return;
  }

  if (IsLocked())
  {
    LOG_DEBUG(logger) << "Failed to lock screen: the screen is already locked.";
    return;
  }

  std::for_each(fade_windows_.begin(), fade_windows_.end(), [](nux::ObjectPtr<nux::BaseWindow> const& window) {
    window->ShowWindow(false);
  });

  fade_windows_.clear();
  animation::SetValue(fade_windows_animator_, animation::Direction::BACKWARD);

  lockscreen_timeout_.reset(new glib::Timeout(10, [this](){

    bool grabbed_by_fade = false;

    std::for_each(fade_windows_.begin(), fade_windows_.end(), [&grabbed_by_fade](nux::ObjectPtr<nux::BaseWindow> const& window) {
      grabbed_by_fade = grabbed_by_fade || window->OwnsPointerGrab();
    });


    if (WindowManager::Default().IsScreenGrabbed() && !grabbed_by_fade)
    {
      LOG_DEBUG(logger) << "Failed to lock the screen: the screen is already grabbed.";
      return true; // keep trying
    }

    LockScreenUsingUnity();
    session_manager_->locked.emit();

    return false;
  }));
}

void Controller::LockScreenUsingUnity()
{
  indicators_ = std::make_shared<indicator::LockScreenDBusIndicators>();
  upstart_wrapper_->Emit("desktop-lock");

  ShowShields();
}

void Controller::ShowShields()
{
  old_blur_type_ = BackgroundEffectHelper::blur_type;
  BackgroundEffectHelper::blur_type = BLUR_NONE;

  WindowManager::Default().SaveInputFocus();
  EnsureShields(UScreen::GetDefault()->GetMonitors());
  uscreen_connection_->unblock();

  std::for_each(shields_.begin(), shields_.end(), [] (nux::ObjectPtr<Shield> const& shield) {
    shield->SetOpacity(0.0f);
    shield->ShowWindow(true);
    shield->PushToFront();
  });

  animation::StartOrReverse(fade_animator_, animation::Direction::FORWARD);
}

void Controller::OnUnlockRequested()
{
  lockscreen_timeout_.reset();

  if (!IsLocked())
    return;

  HideShields();
}

void Controller::HideShields()
{
  std::for_each(shields_.begin(), shields_.end(), [](nux::ObjectPtr<Shield> const& shield) {
    shield->UnGrabPointer();
    shield->UnGrabKeyboard();
  });

  WindowManager::Default().RestoreInputFocus();
  animation::StartOrReverse(fade_animator_, animation::Direction::BACKWARD);

  BackgroundEffectHelper::blur_type = old_blur_type_;
}

bool Controller::IsLocked() const
{
  return !shields_.empty();
}

double Controller::Opacity() const
{
  return fade_animator_.GetCurrentValue();
}

bool Controller::HasOpenMenu() const
{
  return primary_shield_.IsValid() ? primary_shield_->IsIndicatorOpen() : false;
}

} // lockscreen
} // unity
