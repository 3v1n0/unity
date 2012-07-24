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
 * Authored by: Marco Trevisan (Treviño) <marco.trevisan@canonical.com>
 *
 */

#include <gmock/gmock.h>
#include "test_utils.h"

#include "FavoriteStore.h"
#include "MultiMonitor.h"
#include "LauncherController.h"
#include "Launcher.h"
#include "PanelStyle.h"
#include "UnitySettings.h"
#include "UScreen.h"

using namespace unity;
using namespace unity::launcher;
using namespace testing;

namespace
{

class MockFavoriteStore : public FavoriteStore
{
public:
  FavoriteList const& GetFavorites()
  {
    return fav_list_;
  };

  void AddFavorite(std::string const& desktop_path, int position) {};
  void RemoveFavorite(std::string const& desktop_path) {};
  void MoveFavorite(std::string const& desktop_path, int position) {};
  void SetFavorites(FavoriteList const& desktop_paths) {};

private:
  FavoriteList fav_list_;
};

class MockUScreen : public UScreen
{
public:
  MockUScreen()
  {
    Reset(false);
  }

  ~MockUScreen()
  {
    if (default_screen_ == this)
      default_screen_ = nullptr;
  }

  void Reset(bool emit = true)
  {
    default_screen_ = this;
    primary_ = 0;
    monitors_ = {nux::Geometry(0, 0, 1024, 768)};

    changed.emit(primary_, monitors_);
  }

  void SetPrimary(int primary, bool emit = true)
  {
    if (primary_ != primary)
    {
      primary_ = primary;

      if (emit)
        changed.emit(primary_, monitors_);
    }
  }
};

class TestLauncherController : public Test
{
public:
  TestLauncherController()
    : lc(nux::GetGraphicsDisplay()->GetX11Display())
  {}

  virtual void SetUp()
  {
    lc.options = std::make_shared<Options>();
    lc.multiple_launchers = true;
  }

  void SetupFakeMultiMonitor(int primary = 0, bool emit_update = true)
  {
    uscreen.SetPrimary(primary, false);
    uscreen.GetMonitors().clear();
    const unsigned MONITOR_WIDTH = 1024;
    const unsigned MONITOR_HEIGHT = 768;

    for (int i = 0, total_width = 0; i < max_num_monitors; ++i)
    {
      uscreen.GetMonitors().push_back(nux::Geometry(MONITOR_WIDTH, MONITOR_HEIGHT, total_width, 0));
      total_width += MONITOR_WIDTH;

      if (emit_update)
        uscreen.changed.emit(uscreen.GetPrimaryMonitor(), uscreen.GetMonitors());
    }
  }

  MockUScreen uscreen;
  Settings settings;
  panel::Style panel_style;
  MockFavoriteStore favorite_store;
  Controller lc;
};

TEST_F(TestLauncherController, Construction)
{
  EXPECT_NE(lc.options(), nullptr);
  EXPECT_TRUE(lc.multiple_launchers());
}

TEST_F(TestLauncherController, MultimonitorMultipleLaunchers)
{
  lc.multiple_launchers = true;
  SetupFakeMultiMonitor();

  ASSERT_EQ(lc.launchers().size(), max_num_monitors);

  for (int i = 0; i < max_num_monitors; ++i)
  {
    EXPECT_EQ(lc.launchers()[i]->monitor(), i);
  }
}

TEST_F(TestLauncherController, MultimonitorSingleLauncher)
{
  lc.multiple_launchers = false;
  SetupFakeMultiMonitor(0, false);

  for (int i = 0; i < max_num_monitors; ++i)
  {
    uscreen.SetPrimary(i);
    ASSERT_EQ(lc.launchers().size(), 1);
    EXPECT_EQ(lc.launcher().monitor(), i);
  }
}

TEST_F(TestLauncherController, MultimonitorSwitchToMultipleLaunchers)
{
  lc.multiple_launchers = false;
  SetupFakeMultiMonitor();

  ASSERT_EQ(lc.launchers().size(), 1);

  lc.multiple_launchers = true;
  EXPECT_EQ(lc.launchers().size(), max_num_monitors);
}

TEST_F(TestLauncherController, MultimonitorSwitchToSingleLauncher)
{
  lc.multiple_launchers = true;
  int primary = 3;
  SetupFakeMultiMonitor(primary);

  ASSERT_EQ(lc.launchers().size(), max_num_monitors);

  lc.multiple_launchers = false;
  EXPECT_EQ(lc.launchers().size(), 1);
  EXPECT_EQ(lc.launcher().monitor(), primary);
}

TEST_F(TestLauncherController, MultimonitorSwitchToSingleMonitor)
{
  SetupFakeMultiMonitor();
  ASSERT_EQ(lc.launchers().size(), max_num_monitors);

  uscreen.Reset();
  EXPECT_EQ(lc.launchers().size(), 1);
  EXPECT_EQ(lc.launcher().monitor(), 0);
}

TEST_F(TestLauncherController, MultimonitorRemoveMiddleMonitor)
{
  SetupFakeMultiMonitor();
  ASSERT_EQ(lc.launchers().size(), max_num_monitors);

  std::vector<nux::Geometry> &monitors = uscreen.GetMonitors();
  monitors.erase(monitors.begin() + monitors.size()/2);
  uscreen.changed.emit(uscreen.GetPrimaryMonitor(), uscreen.GetMonitors());
  ASSERT_EQ(lc.launchers().size(), max_num_monitors - 1);

  for (int i = 0; i < max_num_monitors - 1; ++i)
    EXPECT_EQ(lc.launchers()[i]->monitor(), i);
}

TEST_F(TestLauncherController, SingleMonitorSwitchToMultimonitor)
{
  ASSERT_EQ(lc.launchers().size(), 1);

  SetupFakeMultiMonitor();

  EXPECT_EQ(lc.launchers().size(), max_num_monitors);
}

}
