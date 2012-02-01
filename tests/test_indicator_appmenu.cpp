// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012 Canonical Ltd
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
 * Authored by: Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 */

#include <UnityCore/AppmenuIndicator.h>

#include <gtest/gtest.h>

using namespace std;
using namespace unity;
using namespace indicator;

namespace
{

TEST(TestAppmenuIndicator, Construction)
{
  g_setenv("GSETTINGS_BACKEND", "memory", true);

  AppmenuIndicator indicator("indicator-appmenu");

  EXPECT_EQ(indicator.name(), "indicator-appmenu");
  EXPECT_TRUE(indicator.IsAppmenu());
  EXPECT_FALSE(indicator.IsIntegrated());
}

TEST(TestAppmenuIndicator, ConstructionIntegrated)
{
  g_setenv("GSETTINGS_BACKEND", "memory", true);

  glib::Object<GSettings> gsettings(g_settings_new("com.canonical.indicator.appmenu"));
  g_settings_set_string(gsettings, "menu-mode", "locally-integrated");

  AppmenuIndicator indicator("indicator-appmenu-integrated");

  EXPECT_EQ(indicator.name(), "indicator-appmenu-integrated");
  EXPECT_TRUE(indicator.IsAppmenu());
  EXPECT_TRUE(indicator.IsIntegrated());
}

TEST(TestAppmenuIndicator, IntegratedValue)
{
  g_setenv("GSETTINGS_BACKEND", "memory", true);

  glib::Object<GSettings> gsettings(g_settings_new("com.canonical.indicator.appmenu"));
  g_settings_set_string(gsettings, "menu-mode", "global");

  AppmenuIndicator indicator("indicator-appmenu");
  EXPECT_FALSE(indicator.IsIntegrated());

  bool integrated_changed = false;
  bool integrated_value = false;
  indicator.integrated_changed.connect([&] (bool new_value) {
    integrated_changed = true;
    integrated_value = new_value;
  });

  g_settings_set_string(gsettings, "menu-mode", "locally-integrated");
  EXPECT_TRUE(integrated_changed);
  EXPECT_TRUE(integrated_value);
  EXPECT_TRUE(indicator.IsIntegrated());

  integrated_changed = false;
  g_settings_set_string(gsettings, "menu-mode", "locally-integrated");
  EXPECT_FALSE(integrated_changed);
  EXPECT_TRUE(integrated_value);
  EXPECT_TRUE(indicator.IsIntegrated());

  integrated_changed = false;
  g_settings_set_string(gsettings, "menu-mode", "global");
  EXPECT_TRUE(integrated_changed);
  EXPECT_FALSE(integrated_value);
  EXPECT_FALSE(indicator.IsIntegrated());
}

TEST(TestAppmenuIndicator, ShowAppmenu)
{
  g_setenv("GSETTINGS_BACKEND", "memory", true);

  AppmenuIndicator indicator("indicator-appmenu");

  int show_x, show_y;
  unsigned int show_xid, show_timestamp;

  // Connecting to signals
  indicator.on_show_appmenu.connect([&] (unsigned int xid, int x, int y,
                                         unsigned int timestamp) {
    show_xid = xid;
    show_x = x;
    show_y = y;
    show_timestamp = timestamp;
  });

  indicator.ShowAppmenu(123456789, 50, 100, 1328063758);
  EXPECT_EQ(show_xid, 123456789);
  EXPECT_EQ(show_x, 50);
  EXPECT_EQ(show_y, 100);
  EXPECT_EQ(show_timestamp, 1328063758);
}

}
