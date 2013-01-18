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
  AppmenuIndicator indicator("indicator-appmenu");

  EXPECT_EQ(indicator.name(), "indicator-appmenu");
  EXPECT_TRUE(indicator.IsAppmenu());
}

TEST(TestAppmenuIndicator, ShowAppmenu)
{
  AppmenuIndicator indicator("indicator-appmenu");

  bool signal_emitted = false;
  int show_x, show_y;
  unsigned show_xid;

  // Connecting to signals
  indicator.on_show_appmenu.connect([&] (unsigned int xid, int x, int y) {
    signal_emitted = true;
    show_xid = xid;
    show_x = x;
    show_y = y;
  });

  indicator.ShowAppmenu(123456789, 50, 100);
  EXPECT_TRUE(signal_emitted);

  EXPECT_EQ(show_xid, 123456789);
  EXPECT_EQ(show_x, 50);
  EXPECT_EQ(show_y, 100);
}

}
