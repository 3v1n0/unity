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
#include "unity-shared/UnitySettings.h"
#include "test_utils.h"

using namespace unity;
using namespace testing;


namespace
{

class TestDashController : public Test
{
public:
  TestDashController()
  : base_window_(new testmocks::MockBaseWindow([](nux::Geometry const& geo)
                                               { return geo; }))
  { }

  virtual void SetUp()
  {
    // Set expectations for creating the controller
    EXPECT_CALL(*base_window_, SetOpacity(0.0f))
      .WillOnce(Invoke(base_window_.GetPointer(),
                       &testmocks::MockBaseWindow::RealSetOpacity));

    controller_.reset(new dash::Controller([&](){ return base_window_.GetPointer();}));
    Mock::VerifyAndClearExpectations(base_window_.GetPointer());
  }

protected:
  dash::Controller::Ptr controller_;
  testmocks::MockBaseWindow::Ptr base_window_;

  // required to create hidden secret global variables
  Settings unity_settings_;
  dash::Style dash_style_;
  panel::Style panel_style_;
};


TEST_F(TestDashController, TestShowAndHideDash)
{
  // Set expectations for showing the Dash
  EXPECT_CALL(*base_window_, SetOpacity(_)).Times(AnyNumber());
  EXPECT_CALL(*base_window_, SetOpacity(Eq(1.0f)))
      .WillOnce(Invoke(base_window_.GetPointer(),
                       &testmocks::MockBaseWindow::RealSetOpacity));

  controller_->ShowDash();
  Utils::WaitForTimeout(1);
  Mock::VerifyAndClearExpectations(base_window_.GetPointer());

  // Set expectations for hiding the Dash
  EXPECT_CALL(*base_window_, SetOpacity(_)).Times(AnyNumber());
  EXPECT_CALL(*base_window_, SetOpacity(Eq(0.0f))).Times(1);

  controller_->HideDash();
  Utils::WaitForTimeout(1);
}

}
