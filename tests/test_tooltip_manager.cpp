// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2013 Canonical Ltd
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
 * Authored by: Jacob Edwards <j.johan.edwards@gmail.com>
 */

#include <gtest/gtest.h>
using namespace testing;

#include "launcher/TooltipManager.h"
#include "launcher/MockLauncherIcon.h"
#include "test_utils.h"

namespace unity
{
namespace launcher
{

namespace {

TEST(TestTooltipManager, TestHideAndShowTooltip) {
  // Makes sure that TooltipManager calls icon->ShowTooltip() when the mouse
  // hovers it, and icon->HideTooltip() after the mouse dehovers it.
  TooltipManager tm;
  MockLauncherIcon* icon = new MockLauncherIcon();

  tm.SetIcon((AbstractLauncherIcon::Ptr)icon);
  tm.MouseMoved();
  Utils::WaitForTimeoutMSec(1050);

  EXPECT_TRUE(icon->IsTooltipVisible());
  tm.SetIcon((AbstractLauncherIcon::Ptr)nullptr);
  EXPECT_FALSE(icon->IsTooltipVisible());
}

}

}
}
