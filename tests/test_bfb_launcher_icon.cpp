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

#include "unity-shared/UnitySettings.h"

#include "BFBLauncherIcon.h"

using namespace unity;
using namespace unity::launcher;

namespace
{

class MockBFBLauncherIcon : public BFBLauncherIcon
{
public:
  MockBFBLauncherIcon()
    : BFBLauncherIcon(LauncherHideMode::LAUNCHER_HIDE_NEVER)
  {}
};

struct TestBFBLauncherIcon : testing::Test
{
  unity::Settings settings;
  MockBFBLauncherIcon bfb;
};

TEST_F(TestBFBLauncherIcon, Position)
{
  EXPECT_EQ(bfb.position, AbstractLauncherIcon::Position::BEGIN);
}

TEST_F(TestBFBLauncherIcon, OverlayMenus)
{
  for (auto menu_item : bfb.Menus())
  {
    bool overlay_item = dbusmenu_menuitem_property_get_bool(menu_item, QuicklistMenuItem::OVERLAY_MENU_ITEM_PROPERTY);
    ASSERT_TRUE(overlay_item);
  }
}

}
