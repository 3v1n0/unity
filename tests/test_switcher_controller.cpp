/*
 * Copyright 2012 Canonical Ltd.
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

#include <gtest/gtest.h>
#include <time.h>

#include "test_utils.h"

#include "SwitcherController.h"
#include "DesktopLauncherIcon.h"
#include "TimeUtil.h"


using namespace unity::switcher;

namespace
{

unsigned int DEFAULT_LAZY_CONSTRUCT_TIMEOUT = 20;

class MockSwitcherController : public Controller
{
public:
  MockSwitcherController()
    : Controller()
    , window_constructed_(false)
    , view_constructed_(false)
    , view_shown_(false)
    , detail_timeout_reached_(false)
  {};

  MockSwitcherController(unsigned int load_timeout)
    : Controller(load_timeout)
    , window_constructed_(false)
    , view_constructed_(false)
    , view_shown_(false)
    , detail_timeout_reached_(false)
  {};

  virtual void ConstructWindow()
  {
    window_constructed_ = true;
  }

  virtual void ConstructView()
  {
    view_constructed_ = true;
  }

  virtual void ShowView()
  {
    view_shown_ = true;
  }

  virtual bool OnDetailTimer()
  {
    detail_timeout_reached_ = true;
    clock_gettime(CLOCK_MONOTONIC, &detail_timespec_);
    return false;
  }

  unsigned int GetConstructTimeout() const
  {
    return construct_timeout_;
  }

  void FakeSelectionChange()
  {
    unity::launcher::AbstractLauncherIcon::Ptr icon;
    OnModelSelectionChanged(icon);
  }

  bool window_constructed_;
  bool view_constructed_;
  bool view_shown_;
  bool detail_timeout_reached_;
  struct timespec detail_timespec_;
};

TEST(TestSwitcherController, Construction)
{
  Controller controller;
  EXPECT_FALSE(controller.Visible());
}

TEST(TestSwitcherController, LazyConstructionTimeoutLength)
{
  MockSwitcherController controller;
  EXPECT_EQ(controller.GetConstructTimeout(), DEFAULT_LAZY_CONSTRUCT_TIMEOUT);
}

TEST(TestSwitcherController, LazyWindowConstruction)
{
  // Setting the timeout to a lower value to speed-up the test
  MockSwitcherController controller(2);

  EXPECT_EQ(controller.GetConstructTimeout(), 2);

  g_timeout_add_seconds(controller.GetConstructTimeout()/2, [] (gpointer data) -> gboolean {
    auto controller = static_cast<MockSwitcherController*>(data);
    EXPECT_FALSE(controller->window_constructed_);

    return FALSE;
  }, &controller);

  Utils::WaitUntil(controller.window_constructed_, controller.GetConstructTimeout() + 1);
  EXPECT_TRUE(controller.window_constructed_);
}

TEST(TestSwitcherController, InitialDetailTimeout)
{
  MockSwitcherController controller;
  std::vector<unity::launcher::AbstractLauncherIcon::Ptr> results;
  results.push_back(unity::launcher::AbstractLauncherIcon::Ptr(new unity::launcher::DesktopLauncherIcon()));
  struct timespec current;

  controller.initial_detail_timeout_length = 2000;
  controller.detail_timeout_length = 20000;
  clock_gettime(CLOCK_MONOTONIC, &current);

  controller.Show(ShowMode::ALL, SortMode::LAUNCHER_ORDER, results);

  Utils::WaitUntil(controller.detail_timeout_reached_, 3);
  ASSERT_TRUE(controller.detail_timeout_reached_);
  EXPECT_TRUE(unity::TimeUtil::TimeDelta(&controller.detail_timespec_, &current) >= 2000);
}

TEST(TestSwitcherController, DetailTimeout)
{
  MockSwitcherController controller;
  struct timespec current;

  controller.detail_timeout_length = 1000;
  controller.initial_detail_timeout_length = 10000;
  clock_gettime(CLOCK_MONOTONIC, &current);

  controller.FakeSelectionChange();

  Utils::WaitUntil(controller.detail_timeout_reached_, 2);
  ASSERT_TRUE(controller.detail_timeout_reached_);
  EXPECT_TRUE(unity::TimeUtil::TimeDelta(&controller.detail_timespec_, &current) >= 1000);
}

TEST(TestSwitcherController, ShowSwitcher)
{
  MockSwitcherController controller;
  std::vector<unity::launcher::AbstractLauncherIcon::Ptr> results;
  results.push_back(unity::launcher::AbstractLauncherIcon::Ptr(new unity::launcher::DesktopLauncherIcon()));

  controller.Show(ShowMode::ALL, SortMode::LAUNCHER_ORDER, results);

  Utils::WaitUntil(controller.view_shown_, 2);
  ASSERT_TRUE(controller.view_shown_);
}

TEST(TestSwitcherController, ShowSwitcherNoShowDeskop)
{
  MockSwitcherController controller;
  controller.SetShowDesktopDisabled(true);

  ASSERT_TRUE(controller.IsShowDesktopDisabled());
  ASSERT_TRUE(controller.StartIndex() == 0);
}

TEST(TestSwitcherController, ShowSwitcherNoResults)
{
  MockSwitcherController controller;
  controller.SetShowDesktopDisabled(true);
  std::vector<unity::launcher::AbstractLauncherIcon::Ptr> results;

  controller.Show(ShowMode::CURRENT_VIEWPORT, SortMode::FOCUS_ORDER, results);

  ASSERT_FALSE(controller.Visible());
}

}
