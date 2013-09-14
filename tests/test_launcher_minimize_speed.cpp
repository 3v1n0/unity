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
 * Authored by: Ugo Riboni <ugo.riboni@canonical.com>
 *
 */

#include <config.h>

#include <gio/gio.h>
#include <gtest/gtest.h>
#include "test_utils.h"

#include "plugins/unityshell/src/WindowMinimizeSpeedController.h"

using namespace unity;
using namespace testing;

namespace
{

class TestLauncherMinimizeSpeed : public Test
{
public:
  glib::Object<GSettings> mSettings;
  WindowMinimizeSpeedController* mController;
  
  /* override */ void SetUp()
  {
    Utils::init_gsettings_test_environment();
    mSettings = g_settings_new("com.canonical.Unity");
    mController = new WindowMinimizeSpeedController();
  }

  /* override */ void TearDown()
  {
    Utils::reset_gsettings_test_environment();
    delete mController;
  }
};

TEST_F(TestLauncherMinimizeSpeed, TestSlowest)
{
    g_settings_set_int(mSettings, "minimize-count", 0);
    g_settings_set_int(mSettings, "minimize-speed-threshold", 10);
    g_settings_set_int(mSettings, "minimize-fast-duration", 200);
    g_settings_set_int(mSettings, "minimize-slow-duration", 1200);
    
    EXPECT_TRUE(mController->getDuration() == 1200);
}

TEST_F(TestLauncherMinimizeSpeed, TestFastest)
{
    g_settings_set_int(mSettings, "minimize-count", 10);
    g_settings_set_int(mSettings, "minimize-speed-threshold", 10);
    g_settings_set_int(mSettings, "minimize-fast-duration", 200);
    g_settings_set_int(mSettings, "minimize-slow-duration", 1200);
    
    EXPECT_TRUE(mController->getDuration() == 200);
}

TEST_F(TestLauncherMinimizeSpeed, TestHalfway)
{
    g_settings_set_int(mSettings, "minimize-count", 5);
    g_settings_set_int(mSettings, "minimize-speed-threshold", 10);
    g_settings_set_int(mSettings, "minimize-fast-duration", 200);
    g_settings_set_int(mSettings, "minimize-slow-duration", 1200);
    
    EXPECT_TRUE(mController->getDuration() == 700);
}

TEST_F(TestLauncherMinimizeSpeed, TestOvershoot)
{
    g_settings_set_int(mSettings, "minimize-count", 20);
    g_settings_set_int(mSettings, "minimize-speed-threshold", 10);
    g_settings_set_int(mSettings, "minimize-fast-duration", 200);
    g_settings_set_int(mSettings, "minimize-slow-duration", 1200);
    
    EXPECT_TRUE(mController->getDuration() == 200);
}

TEST_F(TestLauncherMinimizeSpeed, TestSignal)
{

  bool signal_emitted = false;
  mController->DurationChanged.connect([&] () {
    signal_emitted = true;
  });
  
  g_settings_set_int(mSettings, "minimize-count", 5);
  EXPECT_TRUE(signal_emitted);
}

TEST_F(TestLauncherMinimizeSpeed, TestInvalidFastSlow)
{
  g_settings_set_int(mSettings, "minimize-fast-duration", 2000);
  g_settings_set_int(mSettings, "minimize-slow-duration", 100);

  bool signal_emitted = false;
  mController->DurationChanged.connect([&] () {
    signal_emitted = true;
  });
  
  g_settings_set_int(mSettings, "minimize-count", 5);
  EXPECT_FALSE(signal_emitted);
}
}