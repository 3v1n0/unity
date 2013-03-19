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
 *              Andrea Azzarone <andrea.azzarone@gmail.com>
 */

#include <gmock/gmock.h>
using namespace testing;

#include "launcher/TooltipManager.h"
#include "launcher/MockLauncherIcon.h"
using unity::launcher::MockLauncherIcon;
using unity::launcher::TooltipManager;

#include "test_utils.h"

namespace
{

bool CheckIsTooltipVisible(nux::ObjectPtr<MockLauncherIcon> const& icon, bool value) {
  return icon->IsTooltipVisible() == value;
}

struct TestTooltipManager : public Test {
  TooltipManager tooltip_manager;
};

TEST_F(TestTooltipManager, MouseMoved)
{
  nux::ObjectPtr<MockLauncherIcon> icon1(new MockLauncherIcon());
  nux::ObjectPtr<MockLauncherIcon> icon2(new MockLauncherIcon());

  tooltip_manager.MouseMoved(icon1);
  ASSERT_FALSE(icon1->IsTooltipVisible()); // don't skip the timeout!
  Utils::WaitUntil(std::bind(CheckIsTooltipVisible, icon1, true));
  Utils::WaitUntil(std::bind(CheckIsTooltipVisible, icon2, false));

  tooltip_manager.MouseMoved(icon2);
  ASSERT_FALSE(icon1->IsTooltipVisible());
  ASSERT_TRUE(icon2->IsTooltipVisible());

  tooltip_manager.MouseMoved(nux::ObjectPtr<MockLauncherIcon>());
  ASSERT_FALSE(icon1->IsTooltipVisible());
  ASSERT_FALSE(icon2->IsTooltipVisible());

  tooltip_manager.MouseMoved(icon1);
  ASSERT_TRUE(icon1->IsTooltipVisible());
  ASSERT_FALSE(icon2->IsTooltipVisible());
}

TEST_F(TestTooltipManager, IconClicked)
{
  nux::ObjectPtr<MockLauncherIcon> icon(new MockLauncherIcon());

  tooltip_manager.MouseMoved(icon);
  ASSERT_FALSE(icon->IsTooltipVisible()); // don't skip the timeout!
  Utils::WaitUntil(std::bind(CheckIsTooltipVisible, icon, true));

  tooltip_manager.IconClicked();
  ASSERT_FALSE(icon->IsTooltipVisible());
}

TEST_F(TestTooltipManager, SetHover)
{
  nux::ObjectPtr<MockLauncherIcon> icon(new MockLauncherIcon());

  tooltip_manager.MouseMoved(icon);
  ASSERT_FALSE(icon->IsTooltipVisible()); // don't skip the timeout!
  Utils::WaitUntil(std::bind(CheckIsTooltipVisible, icon, true));

  tooltip_manager.SetHover(false);
  ASSERT_FALSE(icon->IsTooltipVisible());

  tooltip_manager.MouseMoved(icon);
  ASSERT_FALSE(icon->IsTooltipVisible()); // don't skip the timeout!
  Utils::WaitUntil(std::bind(CheckIsTooltipVisible, icon, true));
}

}
