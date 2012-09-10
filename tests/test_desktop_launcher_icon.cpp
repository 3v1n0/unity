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
 */

#include <gmock/gmock.h>

#include "DesktopLauncherIcon.h"

using namespace unity;
using namespace unity::launcher;

namespace
{

struct TestDesktopLauncherIcon : testing::Test
{
  DesktopLauncherIcon icon;
};

TEST_F(TestDesktopLauncherIcon, Type)
{
  EXPECT_EQ(icon.GetIconType(), AbstractLauncherIcon::IconType::DESKTOP);
}

TEST_F(TestDesktopLauncherIcon, Shortcut)
{
  EXPECT_EQ(icon.GetShortcut(), 'd');
}

TEST_F(TestDesktopLauncherIcon, Position)
{
  EXPECT_EQ(icon.position(), AbstractLauncherIcon::Position::FLOATING);
}

TEST_F(TestDesktopLauncherIcon, ShowInSwitcher)
{
  EXPECT_TRUE(icon.ShowInSwitcher(false));
  EXPECT_TRUE(icon.ShowInSwitcher(true));

  icon.SetShowInSwitcher(false);

  EXPECT_FALSE(icon.ShowInSwitcher(false));
  EXPECT_FALSE(icon.ShowInSwitcher(true));
}

}
