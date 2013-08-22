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
 * Authored by: Marco Trevisan (Treviño) <marco.trevisan@canonical.com>
 */

#include <gmock/gmock.h>
#include <Nux/NuxTimerTickSource.h>
#include <NuxCore/AnimationController.h>
#include "test_mock_session_manager.h"
#include "SessionController.h"
#include "UBusMessages.h"
#include "UnitySettings.h"
#include "test_utils.h"

namespace unity
{
namespace session
{

const unsigned ANIMATION_DURATION = 90 * 1000; // in microseconds

struct TestSessionController : testing::Test
{
  TestSessionController()
    : animation_controller(tick_source)
    , manager(std::make_shared<testing::NiceMock<MockManager>>())
    , controller(manager)
  {
    ON_CALL(*manager, CanShutdown()).WillByDefault(testing::Return(true));
  }

  struct ControllerWrap : Controller
  {
    ControllerWrap(Manager::Ptr const& manager) : Controller(manager) {}

    using Controller::view_;
    using Controller::view_window_;
  };

  nux::NuxTimerTickSource tick_source;
  nux::animation::AnimationController animation_controller;
  unity::Settings settings;
  MockManager::Ptr manager ;
  ControllerWrap controller;
};

TEST_F(TestSessionController, Construct)
{
  EXPECT_FALSE(controller.Visible());
}

struct ShowMode : TestSessionController, testing::WithParamInterface<View::Mode> {};
INSTANTIATE_TEST_CASE_P(TestSessionController, ShowMode,
  testing::Values(View::Mode::SHUTDOWN, View::Mode::LOGOUT, View::Mode::FULL));

TEST_P(/*TestSessionController*/ShowMode, Show)
{
  controller.Show(GetParam());
  EXPECT_TRUE(controller.Visible());
  EXPECT_EQ(controller.view_->mode(), GetParam());
  EXPECT_EQ(nux::GetWindowCompositor().GetKeyFocusArea(), controller.view_->key_focus_area());
  EXPECT_TRUE(controller.view_->live_background());
}

TEST_P(/*TestSessionController*/ShowMode, RequestsHideOverlay)
{
  UBusManager ubus;
  bool request_hide = false;
  ubus.RegisterInterest(UBUS_OVERLAY_CLOSE_REQUEST, [&request_hide] (GVariant*) { request_hide = true; });

  controller.Show(GetParam());
  Utils::WaitUntilMSec(request_hide);
}

TEST_F(TestSessionController, Hide)
{
  controller.Show(View::Mode::FULL);
  ASSERT_TRUE(controller.Visible());
  EXPECT_CALL(*manager, CancelAction()).Times(0);

  controller.Hide();
  tick_source.tick(ANIMATION_DURATION);

  EXPECT_FALSE(controller.Visible());
  EXPECT_FALSE(controller.view_->live_background());
}

struct Inhibited : TestSessionController, testing::WithParamInterface<bool> {};
INSTANTIATE_TEST_CASE_P(TestSessionController, Inhibited, testing::Values(true, false));

TEST_P(/*TestSessionController*/Inhibited, RebootRequested)
{
  manager->reboot_requested.emit(GetParam());
  EXPECT_TRUE(controller.Visible());
  EXPECT_EQ(controller.view_->mode(), View::Mode::SHUTDOWN);
  EXPECT_EQ(controller.view_->have_inhibitors(), GetParam());
}

TEST_P(/*TestSessionController*/Inhibited, ShutdownRequested)
{
  manager->shutdown_requested.emit(GetParam());
  EXPECT_TRUE(controller.Visible());
  EXPECT_EQ(controller.view_->mode(), View::Mode::FULL);
  EXPECT_EQ(controller.view_->have_inhibitors(), GetParam());
}

TEST_P(/*TestSessionController*/Inhibited, LogoutRequested)
{
  manager->logout_requested.emit(GetParam());
  EXPECT_TRUE(controller.Visible());
  EXPECT_EQ(controller.view_->mode(), View::Mode::LOGOUT);
  EXPECT_EQ(controller.view_->have_inhibitors(), GetParam());
}

TEST_F(TestSessionController, CancelRequested)
{
  controller.Show(View::Mode::FULL);
  ASSERT_TRUE(controller.Visible());

  EXPECT_CALL(*manager, CancelAction()).Times(0);
  manager->cancel_requested.emit();
  tick_source.tick(ANIMATION_DURATION);
  EXPECT_FALSE(controller.Visible());
}

TEST_F(TestSessionController, RequestHide)
{
  controller.Show(View::Mode::FULL);
  ASSERT_TRUE(controller.Visible());

  EXPECT_CALL(*manager, CancelAction()).Times(0);
  controller.view_->request_hide.emit();
  tick_source.tick(ANIMATION_DURATION);
  EXPECT_FALSE(controller.Visible());
}

TEST_F(TestSessionController, RequestClose)
{
  controller.Show(View::Mode::FULL);
  ASSERT_TRUE(controller.Visible());

  EXPECT_CALL(*manager, CancelAction());
  controller.view_->request_close.emit();
  tick_source.tick(ANIMATION_DURATION);
  EXPECT_FALSE(controller.Visible());
}

TEST_F(TestSessionController, HidesAndCancelOnClickOutside)
{
  controller.Show(View::Mode::FULL);
  ASSERT_TRUE(controller.Visible());

  EXPECT_CALL(*manager, CancelAction());
  controller.view_window_->mouse_down_outside_pointer_grab_area.emit(0, 0, 0, 0);
  tick_source.tick(ANIMATION_DURATION);
  EXPECT_FALSE(controller.Visible());
}

} // session
} // unity