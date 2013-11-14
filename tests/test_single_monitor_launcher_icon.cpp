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

#include <gtest/gtest.h>

#include "SingleMonitorLauncherIcon.h"

using namespace unity;
using namespace launcher;

namespace
{

TEST(TestSingleMonitorLauncherIcon, Construction)
{
  SingleMonitorLauncherIcon icon(AbstractLauncherIcon::IconType::NONE, 1);

  EXPECT_EQ(icon.GetMonitor(), 1);
  EXPECT_TRUE(icon.IsVisibleOnMonitor(1));
  EXPECT_FALSE(icon.IsVisibleOnMonitor(0));
}

TEST(TestSingleMonitorLauncherIcon, MonitorVisibility)
{
  SingleMonitorLauncherIcon icon(AbstractLauncherIcon::IconType::NONE, 2);

  for (unsigned i = 0; i < monitors::MAX; ++i)
  {
    bool icon_visible = icon.IsVisibleOnMonitor(i);

    if (i == 2)
      EXPECT_TRUE(icon_visible);
    else
      EXPECT_FALSE(icon_visible);
  }
}

}
