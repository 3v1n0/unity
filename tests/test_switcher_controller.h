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

#ifndef TEST_SWITCHER_CONTROLLER_H
#define TEST_SWITCHER_CONTROLLER_H

#include <NuxCore/AnimationController.h>
#include <gmock/gmock.h>
#include <chrono>

#include "test_utils.h"
#include "DesktopLauncherIcon.h"
#include "SimpleLauncherIcon.h"
#include "SwitcherController.h"
#include "SwitcherView.h"
#include "TimeUtil.h"
#include "unity-shared/UnitySettings.h"
#include "mock-base-window.h"
#include "test_standalone_wm.h"

using namespace std::chrono;

typedef std::chrono::high_resolution_clock Clock;

#ifdef ENABLE_DELAYED_TWO_PHASE_CONSTRUCTION_TESTS
unsigned int DEFAULT_LAZY_CONSTRUCT_TIMEOUT = 20;
#endif

const unsigned FADE_DURATION = 80 * 1000; // in microseconds
const unsigned TICK_DURATION = 10 * 1000;

/**
 * A fake ApplicationWindow for verifying selection of the switcher.
 */
class FakeApplicationWindow : public unity::ApplicationWindow
{
public:
  FakeApplicationWindow(Window xid, uint64_t active_number = 0);
  ~FakeApplicationWindow();

  virtual std::string type() const;

  virtual Window window_id() const;
  virtual int monitor() const;
  virtual unity::ApplicationPtr application() const;
  virtual bool Focus() const;
  virtual void Quit() const;

private:
  Window xid_;
};

/**
 * A fake LauncherIcon for verifying selection operations of the switcher.
 */
struct FakeLauncherIcon : unity::launcher::SimpleLauncherIcon
{
  FakeLauncherIcon(std::string const& app_name, bool allow_detail_view, uint64_t priority);

  unity::WindowList Windows() override;
  bool AllowDetailViewInSwitcher() const override;
  uint64_t SwitcherPriority() override;

  bool allow_detail_view_;
  uint64_t priority_;
  unity::WindowList window_list;
};


/**
 * The base test fixture for verifying the Switcher interface.
 */
class TestSwitcherController : public testing::Test
{
protected:
  TestSwitcherController();

  // required to create hidden secret global variables before test objects
  unity::Settings unity_settings_;

  unity::testwrapper::StandaloneWM WM;
  nux::animation::TickSource tick_source_;
  nux::animation::AnimationController animation_controller_;
  unity::testmocks::MockBaseWindow::Ptr mock_window_;
  unity::switcher::Controller::Ptr controller_;
  std::vector<unity::launcher::AbstractLauncherIcon::Ptr> icons_;
};

#endif // TEST_SWITCHER_CONTROLLER_H
