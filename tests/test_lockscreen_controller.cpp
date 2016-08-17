// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2014 Canonical Ltd
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

#include <gmock/gmock.h>
using namespace testing;

#include "lockscreen/LockScreenAbstractPromptView.h"
#include "lockscreen/LockScreenController.h"

#include <Nux/NuxTimerTickSource.h>
#include <NuxCore/AnimationController.h>
#include <UnityCore/GLibDBusServer.h>


#include "lockscreen/LockScreenSettings.h"
#include "lockscreen/ScreenSaverDBusManager.h"
#include "unity-shared/DashStyle.h"
#include "unity-shared/PanelStyle.h"
#include "unity-shared/UScreen.h"
#include "mock_key_grabber.h"
#include "test_mock_session_manager.h"
#include "test_uscreen_mock.h"
#include "test_utils.h"

namespace unity
{
namespace lockscreen
{
namespace
{

const unsigned ANIMATION_DURATION = 400 * 1000; // in microseconds
const unsigned BLANK_ANIMATION_DURATION = 10000 * 1000;
const unsigned TICK_DURATION =  10 * 1000;

}


struct MockShield : BaseShield
{
  MockShield()
    : BaseShield(nullptr, nullptr, nullptr, nux::ObjectPtr<AbstractUserPromptView>(), 0, false)
  {}

  MOCK_CONST_METHOD0(IsIndicatorOpen, bool());
  MOCK_METHOD0(ActivatePanel, void());
  MOCK_CONST_METHOD0(HasGrab, bool());
  MOCK_METHOD0(ShowPrimaryView, void());
};

struct ShieldFactoryMock : ShieldFactoryInterface
{
  nux::ObjectPtr<BaseShield> CreateShield(session::Manager::Ptr const&,
                                          indicator::Indicators::Ptr const&,
                                          Accelerators::Ptr const&,
                                          nux::ObjectPtr<AbstractUserPromptView> const&,
                                          int, bool) override
  {
    return nux::ObjectPtr<BaseShield>(new MockShield());
  }
};

struct TestLockScreenController : Test
{
  TestLockScreenController()
    : animation_controller(tick_source)
    , session_manager(std::make_shared<NiceMock<session::MockManager>>())
    , key_grabber(std::make_shared<key::MockGrabber::Nice>())
    , dbus_manager(std::make_shared<DBusManager>(session_manager))
    , upstart_wrapper(std::make_shared<UpstartWrapper>())
    , shield_factory(std::make_shared<ShieldFactoryMock>())
    , controller(dbus_manager, session_manager, key_grabber, upstart_wrapper, shield_factory)
  {}

  struct ControllerWrap : Controller
  {
    ControllerWrap(DBusManager::Ptr const& dbus_manager,
                   session::Manager::Ptr const& session_manager,
                   key::Grabber::Ptr const& key_grabber,
                   UpstartWrapper::Ptr const& upstart_wrapper,
                   ShieldFactoryInterface::Ptr const& shield_factory)
      : Controller(dbus_manager, session_manager, key_grabber, upstart_wrapper, shield_factory, /* test_mode */ true)
    {}

    using Controller::shields_;
    using Controller::blank_window_;
  };

  nux::NuxTimerTickSource tick_source;
  nux::animation::AnimationController animation_controller;

  MockUScreen uscreen;
  unity::dash::Style dash_style;
  unity::panel::Style panel_style;
  unity::lockscreen::Settings lockscreen_settings;
  session::MockManager::Ptr session_manager;
  key::MockGrabber::Ptr key_grabber;
  DBusManager::Ptr dbus_manager;
  unity::UpstartWrapper::Ptr upstart_wrapper;

  ShieldFactoryMock::Ptr shield_factory;
  ControllerWrap controller;
};

TEST_F(TestLockScreenController, Construct)
{
  EXPECT_TRUE(controller.shields_.empty());
}

TEST_F(TestLockScreenController, DisconnectUScreenSignalsOnDestruction)
{
  size_t before = uscreen.changed.size();
  {
    Controller dummy(dbus_manager, session_manager, key_grabber);
  }
  ASSERT_EQ(before, uscreen.changed.size());

  std::vector<nux::Geometry> monitors;
  uscreen.changed.emit(0, monitors);
}

TEST_F(TestLockScreenController, DisconnectSessionManagerSignalsOnDestruction)
{
  size_t before = session_manager->unlock_requested.size();
  {
    Controller dummy(dbus_manager, session_manager, key_grabber);
  }
  ASSERT_EQ(before, session_manager->unlock_requested.size());

  session_manager->unlock_requested.emit();
}

TEST_F(TestLockScreenController, UScreenChangedIgnoredOnScreenUnlocked)
{
  uscreen.SetupFakeMultiMonitor(/*primary*/ 0, /*emit_change*/ true);
  EXPECT_TRUE(controller.shields_.empty());
}

TEST_F(TestLockScreenController, LockScreenOnSingleMonitor)
{
  session_manager->lock_requested.emit();

  Utils::WaitUntilMSec([this]{ return controller.shields_.size() == 1; });
  Utils::WaitUntilMSec([this]{ return uscreen.GetMonitors().at(0) == controller.shields_.at(0)->GetGeometry(); });
}

TEST_F(TestLockScreenController, LockScreenOnMultiMonitor)
{
  uscreen.SetupFakeMultiMonitor();

  session_manager->lock_requested.emit();

  Utils::WaitUntilMSec([this]{ return controller.shields_.size() == monitors::MAX; });
  ASSERT_EQ(monitors::MAX, controller.shields_.size());

  for (unsigned int i=0; i < monitors::MAX; ++i)
  {
    Utils::WaitUntilMSec([this, i]{ return uscreen.GetMonitors().at(i) == controller.shields_.at(i)->GetGeometry(); });
    EXPECT_EQ(uscreen.GetMonitors().at(i), controller.shields_.at(i)->GetAbsoluteGeometry());
  }
}

TEST_F(TestLockScreenController, SwitchToMultiMonitor)
{
  session_manager->lock_requested.emit();

  Utils::WaitUntilMSec([this]{ return controller.shields_.size() == 1; });
  ASSERT_EQ(1, controller.shields_.size());

  Utils::WaitUntilMSec([this]{ return uscreen.GetMonitors().at(0) == controller.shields_.at(0)->GetGeometry(); });
  EXPECT_EQ(uscreen.GetMonitors().at(0), controller.shields_.at(0)->GetGeometry());

  uscreen.SetupFakeMultiMonitor(/* primary */ 0, /* emit_change */ true);
  ASSERT_EQ(monitors::MAX, controller.shields_.size());

  for (unsigned int i=0; i < monitors::MAX; ++i)
  {
    ASSERT_EQ(uscreen.GetMonitors().at(i), controller.shields_.at(i)->GetAbsoluteGeometry());
  }
}

TEST_F(TestLockScreenController, SwitchToSingleMonitor)
{
  uscreen.SetupFakeMultiMonitor(/* primary */ 0, /* emit_change */ true);
  session_manager->lock_requested.emit();

  Utils::WaitUntilMSec([this]{ return controller.shields_.size() == monitors::MAX; });
  ASSERT_EQ(monitors::MAX, controller.shields_.size());

  for (unsigned int i=0; i < monitors::MAX; ++i)
  {
    Utils::WaitUntilMSec([this, i]{ return uscreen.GetMonitors().at(i) == controller.shields_.at(i)->GetGeometry(); });
    ASSERT_EQ(uscreen.GetMonitors().at(i), controller.shields_.at(i)->GetAbsoluteGeometry());
  }

  uscreen.Reset(/* emit_change */ true);

  ASSERT_EQ(1, controller.shields_.size());
  EXPECT_EQ(uscreen.GetMonitors().at(0), controller.shields_.at(0)->GetGeometry());
}

TEST_F(TestLockScreenController, UnlockScreenOnSingleMonitor)
{
  session_manager->lock_requested.emit();

  Utils::WaitUntilMSec([this]{ return controller.shields_.size() == 1; });
  ASSERT_EQ(1, controller.shields_.size());

  session_manager->unlock_requested.emit();
  tick_source.tick(ANIMATION_DURATION);

  EXPECT_TRUE(controller.shields_.empty());
}

TEST_F(TestLockScreenController, UnlockScreenOnMultiMonitor)
{
  uscreen.SetupFakeMultiMonitor(/* primary */ 0, /* emit_change */ true);
  session_manager->lock_requested.emit();

  Utils::WaitUntilMSec([this]{ return controller.shields_.size() == monitors::MAX; });
  ASSERT_EQ(monitors::MAX, controller.shields_.size());

  session_manager->unlock_requested.emit();
  tick_source.tick(ANIMATION_DURATION);

  EXPECT_TRUE(controller.shields_.empty());
}

TEST_F(TestLockScreenController, ShieldHasGrabAfterBlank)
{

  // Lock...
  session_manager->lock_requested.emit();
  Utils::WaitUntilMSec([this]{ return controller.shields_.size() == 1; });
  ASSERT_EQ(1, controller.shields_.size());

  // ...and let the screen blank.
  session_manager->presence_status_changed.emit(true);
  tick_source.tick(BLANK_ANIMATION_DURATION);

  ASSERT_TRUE(controller.blank_window_->GetOpacity() == 1.0);
  ASSERT_TRUE(controller.blank_window_->OwnsPointerGrab());

  // Wake the screen
  dbus_manager->simulate_activity.emit();

  EXPECT_TRUE(controller.shields_.at(0)->OwnsPointerGrab());
}

} // lockscreen
} // unity
