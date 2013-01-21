/*
 * Copyright 2012-2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 *
 */

#include <gmock/gmock.h>
#include <time.h>

#include "test_utils.h"
//#include "StubSwitcherController.h"
#include "SwitcherController.h"
#include "DesktopLauncherIcon.h"
#include "TimeUtil.h"
#include "unity-shared/UnitySettings.h"

using namespace testing;
using namespace unity;
using namespace unity::switcher;

namespace
{

#ifdef ENABLE_DELAYED_TWO_PHASE_CONSTRUCTION_TESTS
unsigned int DEFAULT_LAZY_CONSTRUCT_TIMEOUT = 20;
#endif


/**
 * A mock Switcher view for verifying drawing operations of the Switcher
 * interface.
 */
class MockWindow : public nux::BaseWindow
{
public:
  MOCK_METHOD2(ShowWindow, void(bool, bool));
};


/**
 * The base test fixture for verifying the Switcher interface.
 */
class TestSwitcherController : public testing::Test
{
protected:
  TestSwitcherController()
    : mock_window_(new NiceMock<MockWindow>())
  {
    auto create_window = [&](){ return mock_window_; };
    controller_.reset(new Controller(create_window));

    icons_.push_back(unity::launcher::AbstractLauncherIcon::Ptr(new unity::launcher::DesktopLauncherIcon()));
  }

  // required to create hidden secret global variables before test objects
  Settings unity_settings_;

  nux::ObjectPtr<MockWindow> mock_window_;
  Controller::Ptr controller_;
  std::vector<unity::launcher::AbstractLauncherIcon::Ptr> icons_;
};


#ifdef ENABLE_DELAYED_TWO_PHASE_CONSTRUCTION_TESTS
TEST_F(TestSwitcherController, LazyConstructionTimeoutLength)
{
  EXPECT_EQ(controller_->GetConstructTimeout(), DEFAULT_LAZY_CONSTRUCT_TIMEOUT);
}

TEST_F(TestSwitcherController, LazyWindowConstruction)
{
  // Setting the timeout to a lower value to speed-up the test
  SwitcherController controller(2);

  EXPECT_EQ(controller.GetConstructTimeout(), 2);

  g_timeout_add_seconds(controller.GetConstructTimeout()/2, [] (gpointer data) -> gboolean {
    auto controller = static_cast<StubSwitcherController*>(data);
    EXPECT_FALSE(controller->window_constructed_);

    return FALSE;
  }, &controller);

  Utils::WaitUntil(controller.window_constructed_, controller.GetConstructTimeout() + 1);
  EXPECT_TRUE(controller.window_constructed_);
}
#endif

TEST_F(TestSwitcherController, InitialDetailTimeout)
{
  static const int initial_details_timeout = 1000;

  controller_->initial_detail_timeout_length = initial_details_timeout;
  controller_->detail_timeout_length = 10 * initial_details_timeout;

  struct timespec current;
  clock_gettime(CLOCK_MONOTONIC, &current);

  controller_->Show(ShowMode::ALL, SortMode::LAUNCHER_ORDER, icons_);

#ifdef ENABLE_TIMEOUT_VIEW_CHANGE_TESTS
  Utils::WaitUntil(controller_->detail_timeout_reached, 3);
  ASSERT_TRUE(controller_->detail_timeout_reached);
  EXPECT_TRUE(unity::TimeUtil::TimeDelta(&controller_->detail_timespec_, &current) >= 2000);
#endif
}

TEST_F(TestSwitcherController, DetailTimeout)
{
  static const int details_timeout = 1000;

  struct timespec current;

  controller_->detail_timeout_length = details_timeout;
  controller_->initial_detail_timeout_length = 10 * details_timeout;
  clock_gettime(CLOCK_MONOTONIC, &current);

  controller_->Show(ShowMode::ALL, SortMode::LAUNCHER_ORDER, icons_);
  controller_->Next();

#ifdef ENABLE_TIMEOUT_VIEW_CHANGE_TESTS
  Utils::WaitUntil(controller_->detail_timeout_reached, 2);
  ASSERT_TRUE(controller_->detail_timeout_reached);
  EXPECT_TRUE(unity::TimeUtil::TimeDelta(&controller_->detail_timespec_, &current) >= 1000);
#endif
}

TEST_F(TestSwitcherController, ShowSwitcher)
{
  EXPECT_FALSE(controller_->Visible());
  EXPECT_CALL(*mock_window_, ShowWindow(true, _)).Times(AtLeast(1));
  controller_->Show(ShowMode::ALL, SortMode::LAUNCHER_ORDER, icons_);

  Utils::WaitForTimeout(2);

  EXPECT_TRUE(controller_->Visible());
  EXPECT_TRUE(controller_->StartIndex() == 1);
}

TEST_F(TestSwitcherController, ShowSwitcherNoShowDeskop)
{
  controller_->SetShowDesktopDisabled(true);

  ASSERT_TRUE(controller_->IsShowDesktopDisabled());
  ASSERT_TRUE(controller_->StartIndex() == 0);
}

TEST_F(TestSwitcherController, ShowSwitcherNoResults)
{
  controller_->SetShowDesktopDisabled(true);
  std::vector<unity::launcher::AbstractLauncherIcon::Ptr> results;
  EXPECT_CALL(*mock_window_, ShowWindow(true, _)).Times(0);

  controller_->Show(ShowMode::CURRENT_VIEWPORT, SortMode::FOCUS_ORDER, results);

  ASSERT_FALSE(controller_->Visible());
}

}
