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

#include "lockscreen/LockScreenController.h"

#include <Nux/NuxTimerTickSource.h>
#include <NuxCore/AnimationController.h>

#include "unity-shared/PanelStyle.h"
#include "unity-shared/UScreen.h"
#include "unity-shared/UnitySettings.h"
#include "test_mock_session_manager.h"
#include "test_uscreen_mock.h"
#include "test_utils.h"


namespace unity
{
namespace lockscreen
{

const unsigned ANIMATION_DURATION = 200 * 1000; // in microseconds
const unsigned TICK_DURATION =  10 * 1000;

struct ShieldFactoryMock : ShieldFactoryInterface
{
  nux::ObjectPtr<MockableBaseWindow> CreateShield (session::Manager::Ptr const&, bool) override
  {
    nux::ObjectPtr<MockableBaseWindow> shield(new MockableBaseWindow);
    return shield;
  }  
};

struct TestLockScreenController : Test
{
  TestLockScreenController()
    : animation_controller(tick_source)
    , session_manager(std::make_shared<NiceMock<session::MockManager>>())
    , shield_factory(std::make_shared<ShieldFactoryMock>())
    , controller(session_manager, shield_factory)
  {}

  struct ControllerWrap : Controller
  {
    ControllerWrap(session::Manager::Ptr const& session_manager,
                   ShieldFactoryInterface::Ptr const& shield_factory)
      : Controller(session_manager, shield_factory)
    {}

    using Controller::shields_;
  };

  nux::NuxTimerTickSource tick_source;
  nux::animation::AnimationController animation_controller;
  MockUScreen uscreen;
  unity::Settings unity_settings;
  unity::panel::Style panel_style;
  session::MockManager::Ptr session_manager;
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
    Controller dummy(session_manager);
  }
  ASSERT_EQ(before, uscreen.changed.size());

  std::vector<nux::Geometry> monitors;
  uscreen.changed.emit(0, monitors);
}

TEST_F(TestLockScreenController, DisconnectSessionManagerSignalsOnDestruction)
{
  size_t before = session_manager->unlock_requested.size();
  {
    Controller dummy(session_manager);
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

  ASSERT_EQ(1, controller.shields_.size());
  EXPECT_EQ(uscreen.GetMonitors().at(0), controller.shields_.at(0)->GetGeometry());
}

TEST_F(TestLockScreenController, LockScreenOnMultiMonitor)
{
  uscreen.SetupFakeMultiMonitor();

  session_manager->lock_requested.emit();
  ASSERT_EQ(monitors::MAX, controller.shields_.size());

  for (unsigned int i=0; i < monitors::MAX; ++i)
    EXPECT_EQ(uscreen.GetMonitors().at(i), controller.shields_.at(i)->GetAbsoluteGeometry());
}

TEST_F(TestLockScreenController, SwitchToMultiMonitor)
{
  session_manager->lock_requested.emit();

  ASSERT_EQ(1, controller.shields_.size());
  EXPECT_EQ(uscreen.GetMonitors().at(0), controller.shields_.at(0)->GetGeometry());

  uscreen.SetupFakeMultiMonitor(/* primary */ 0, /* emit_change */ true);

  ASSERT_EQ(monitors::MAX, controller.shields_.size());

  for (unsigned int i=0; i < monitors::MAX; ++i)
    EXPECT_EQ(uscreen.GetMonitors().at(i), controller.shields_.at(i)->GetAbsoluteGeometry());  
}

TEST_F(TestLockScreenController, SwitchToSingleMonitor)
{
  uscreen.SetupFakeMultiMonitor(/* primary */ 0, /* emit_change */ true);
  session_manager->lock_requested.emit();

  ASSERT_EQ(monitors::MAX, controller.shields_.size());

  for (unsigned int i=0; i < monitors::MAX; ++i)
    EXPECT_EQ(uscreen.GetMonitors().at(i), controller.shields_.at(i)->GetAbsoluteGeometry());

  uscreen.Reset(/* emit_change */ true);

  ASSERT_EQ(1, controller.shields_.size());
  EXPECT_EQ(uscreen.GetMonitors().at(0), controller.shields_.at(0)->GetGeometry());
}

TEST_F(TestLockScreenController, UnlockScreenOnSingleMonitor)
{
  session_manager->lock_requested.emit();

  ASSERT_EQ(1, controller.shields_.size());

  session_manager->unlock_requested.emit();
  tick_source.tick(ANIMATION_DURATION);

  EXPECT_TRUE(controller.shields_.empty());
}

TEST_F(TestLockScreenController, UnlockScreenOnMultiMonitor)
{
  uscreen.SetupFakeMultiMonitor(/* primary */ 0, /* emit_change */ true);
  session_manager->lock_requested.emit();

  ASSERT_EQ(monitors::MAX, controller.shields_.size());

  session_manager->unlock_requested.emit();
  tick_source.tick(ANIMATION_DURATION);

  EXPECT_TRUE(controller.shields_.empty());
}

} // lockscreen
} // unity
