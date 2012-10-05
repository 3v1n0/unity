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

#include "launcher/ExpoLauncherIcon.h"
#include "unity-shared/WindowManager.h"

using namespace unity;
using namespace unity::launcher;

namespace
{
struct TestExpoLauncherIcon : testing::Test
{
  ExpoLauncherIcon icon;
};

TEST_F(TestExpoLauncherIcon, ActivateToggleExpo)
{
  WindowManager& wm = WindowManager::Default();

  ASSERT_FALSE(wm.IsExpoActive());

  icon.Activate(ActionArg());
  ASSERT_TRUE(wm.IsExpoActive());

  icon.Activate(ActionArg());
  EXPECT_FALSE(wm.IsExpoActive());
}

TEST_F(TestExpoLauncherIcon, Position)
{
  EXPECT_EQ(icon.position(), AbstractLauncherIcon::Position::FLOATING);
}

TEST_F(TestExpoLauncherIcon, RemoteUri)
{
  EXPECT_EQ(icon.RemoteUri(), "unity://expo-icon");
}

}
