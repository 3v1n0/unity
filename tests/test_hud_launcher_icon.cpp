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

#include "HudLauncherIcon.h"

using namespace unity;
using namespace unity::launcher;

namespace
{

class MockHudLauncherIcon : public HudLauncherIcon
{
public:
  MockHudLauncherIcon()
    : HudLauncherIcon(LauncherHideMode::LAUNCHER_HIDE_NEVER)
  {}
};

struct TestHudLauncherIcon : testing::Test
{
  MockHudLauncherIcon hud;
};

TEST_F(TestHudLauncherIcon, Type)
{
  EXPECT_EQ(hud.GetIconType(), AbstractLauncherIcon::IconType::HUD);
}

TEST_F(TestHudLauncherIcon, Position)
{
  EXPECT_EQ(hud.position(), AbstractLauncherIcon::Position::BEGIN);
}

}
