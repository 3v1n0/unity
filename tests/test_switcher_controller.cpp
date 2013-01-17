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
#include "StubSwitcherController.h"
#include "MockSwitcherController.h"
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
  typedef nux::ObjectPtr<MockWindow> Ptr;

  MOCK_METHOD2(ShowWindow, void(bool, bool));
};


/**
 * The base test fixture for verifying the Switcher interface.
 */
class TestSwitcherController : public testing::Test
{
protected:
  TestSwitcherController()
    : mock_window_(new MockWindow())
  {
    auto create_window = [&](){ return mock_window_; };
    Controller::Impl* impl = new ShellController(20, create_window);
    controller_.reset(new Controller([&](){ return Controller::ImplPtr(impl); }));

    icons_.push_back(unity::launcher::AbstractLauncherIcon::Ptr(new unity::launcher::DesktopLauncherIcon()));
  }

  // required to create hidden secret global variables before test objects
  Settings unity_settings_;

  nux::ObjectPtr<MockWindow> mock_window_;
  Controller::Ptr controller_;
  std::vector<unity::launcher::AbstractLauncherIcon::Ptr> icons_;
};


TEST_F(TestSwitcherController, InstantiateMock)
{
  MockSwitcherController mock;
}

TEST_F(TestSwitcherController, InstantiateMockThroughNVI)
{
  MockSwitcherController *mock = new MockSwitcherController;
  Controller controller ([&](){
    return std::unique_ptr<Controller::Impl> (mock);
  });
}

#ifdef ENABLE_DELAYED_TWO_PHASE_CONSTRUCTION_TESTS
TEST_F(TestSwitcherController, LazyConstructionTimeoutLength)
{
  StubSwitcherController controller;
  EXPECT_EQ(controller.GetConstructTimeout(), DEFAULT_LAZY_CONSTRUCT_TIMEOUT);
}

TEST_F(TestSwitcherController, LazyWindowConstruction)
{
  // Setting the timeout to a lower value to speed-up the test
  StubSwitcherController controller(2);

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
  StubSwitcherController controller;

  controller.initial_detail_timeout_length = 2000;
  controller.detail_timeout_length = 20000;

  struct timespec current;
  clock_gettime(CLOCK_MONOTONIC, &current);

  controller.Show(ShowMode::ALL, SortMode::LAUNCHER_ORDER, icons_);

  Utils::WaitUntil(controller.detail_timeout_reached_, 3);
  ASSERT_TRUE(controller.detail_timeout_reached_);
  EXPECT_TRUE(unity::TimeUtil::TimeDelta(&controller.detail_timespec_, &current) >= 2000);
}

TEST_F(TestSwitcherController, DetailTimeout)
{
  StubSwitcherController controller;
  struct timespec current;

  controller.detail_timeout_length = 1000;
  controller.initial_detail_timeout_length = 10000;
  clock_gettime(CLOCK_MONOTONIC, &current);

  controller.FakeSelectionChange();

  Utils::WaitUntil(controller.detail_timeout_reached_, 2);
  ASSERT_TRUE(controller.detail_timeout_reached_);
  EXPECT_TRUE(unity::TimeUtil::TimeDelta(&controller.detail_timespec_, &current) >= 1000);
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
