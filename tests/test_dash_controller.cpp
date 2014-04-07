/*
 * Copyright 2012 Canonical Ltd.
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
 */

#include <gmock/gmock.h>

#include "DashController.h"
#include "mock-base-window.h"
#include "unity-shared/DashStyle.h"
#include "unity-shared/PanelStyle.h"
#include "unity-shared/WindowManager.h"
#include "test_utils.h"

#include <NuxCore/AnimationController.h>

using namespace unity;
using namespace testing;


namespace
{

struct TestDashController : Test
{
  TestDashController()
    : animation_controller(tick_source)
    , base_window_(new testmocks::MockBaseWindow::Nice())
    , controller_(std::make_shared<dash::Controller>([this] { return base_window_.GetPointer(); }))
  {}

protected:
  nux::animation::TickSource tick_source;
  nux::animation::AnimationController animation_controller;

  // required to create hidden secret global variables
  dash::Style dash_style_;
  panel::Style panel_style_;

  testmocks::MockBaseWindow::Ptr base_window_;
  dash::Controller::Ptr controller_;
};

TEST_F(TestDashController, Construction)
{
  EXPECT_CALL(*base_window_, SetOpacity(0.0f));
  controller_ = std::make_shared<dash::Controller>([this] { return base_window_.GetPointer(); });
}

TEST_F(TestDashController, TestShowAndHideDash)
{
  // Verify initial conditions
  EXPECT_EQ(base_window_->GetOpacity(), 0.0);

  // Set expectations for showing the Dash
  {
    InSequence showing;
    EXPECT_CALL(*base_window_, SetOpacity(_)).Times(AtLeast(1));
    EXPECT_CALL(*base_window_, SetOpacity(Eq(1.0f)));
  }

  controller_->ShowDash();
  tick_source.tick.emit(1000*1000);
  Mock::VerifyAndClearExpectations(base_window_.GetPointer());
  EXPECT_EQ(base_window_->GetOpacity(), 1.0);

  // Set expectations for hiding the Dash
  {
    InSequence hiding;
    EXPECT_CALL(*base_window_, SetOpacity(_)).Times(AtLeast(1));
    EXPECT_CALL(*base_window_, SetOpacity(Eq(0.0f)));
  }

  controller_->HideDash();
  tick_source.tick.emit(2000*1000);

  // Verify final conditions
  EXPECT_EQ(base_window_->GetOpacity(), 0.0);
}

TEST_F(TestDashController, DisconnectWMSignalsOnDestruction)
{
  auto& signal = WindowManager::Default().initiate_spread;
  size_t before = signal.size();
  { auto controller = std::make_shared<dash::Controller>([this] { return base_window_.GetPointer(); }); }
  ASSERT_EQ(before, signal.size());
  signal.emit();
}

}
