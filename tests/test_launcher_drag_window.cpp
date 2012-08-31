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

#include "LauncherDragWindow.h"

using namespace unity::launcher;
using namespace testing;

namespace
{
const int ICON_WIDTH = 10;
const int ICON_HEIGHT = 15;
}

namespace unity
{
namespace launcher
{
struct TestLauncherDragWindow : public testing::Test
{
  TestLauncherDragWindow()
    : drag_window(nux::ObjectPtr<nux::IOpenGLBaseTexture>(new nux::IOpenGLBaseTexture(nux::RTTEXTURE, ICON_WIDTH, ICON_HEIGHT, 24, 1, nux::BITFMT_B8G8R8A8)))
  {}

  LauncherDragWindow drag_window;
};
}

TEST_F(TestLauncherDragWindow, Construction)
{
  EXPECT_EQ(drag_window.GetBaseWidth(), ICON_WIDTH);
  EXPECT_EQ(drag_window.GetBaseHeight(), ICON_HEIGHT);
  EXPECT_FALSE(drag_window.Animating());
  EXPECT_FALSE(drag_window.Cancelled());
}

}
