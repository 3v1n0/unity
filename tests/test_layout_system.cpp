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
#include "LayoutSystem.h"
#include "StandaloneWindowManager.h"

namespace unity
{
namespace ui
{
namespace
{
StandaloneWindowManager* wm = nullptr;

StandaloneWindow::Ptr AddFakeWindowToWM(Window xid)
{
  const unsigned top_deco = 5;
  auto fake_window = std::make_shared<StandaloneWindow>(xid);
  fake_window->geo = nux::Geometry(1, 2, 30, 40);
  fake_window->deco_sizes[unsigned(WindowManager::Edge::TOP)] = nux::Size(fake_window->geo.width, top_deco);

  if (!wm)
    wm = dynamic_cast<StandaloneWindowManager*>(&WindowManager::Default());

  wm->AddStandaloneWindow(fake_window);

  return fake_window;
}

TEST(TestLayoutWindow, InitializationNormalWindow)
{
  const Window xid = g_random_int();
  auto fake_window = AddFakeWindowToWM(xid);

  LayoutWindow lwin(xid);
  EXPECT_EQ(lwin.xid, xid);
  EXPECT_EQ(lwin.geo, fake_window->geo);
  EXPECT_EQ(lwin.decoration_height, 0);
  EXPECT_EQ(lwin.selected, false);
  EXPECT_EQ(lwin.aspect_ratio, fake_window->geo.width / static_cast<float>(fake_window->geo.height));
}

TEST(TestLayoutWindow, InitializationMinimizedNormalWindow)
{
  const Window xid = g_random_int();
  auto fake_window = AddFakeWindowToWM(xid);
  wm->Minimize(xid);

  LayoutWindow lwin(xid);
  EXPECT_EQ(lwin.xid, xid);
  EXPECT_EQ(lwin.geo, fake_window->geo);
  EXPECT_EQ(lwin.decoration_height, 0);
  EXPECT_EQ(lwin.selected, false);
  EXPECT_EQ(lwin.aspect_ratio, fake_window->geo.width / static_cast<float>(fake_window->geo.height));
}

TEST(TestLayoutWindow, InitializationMaximizedWindow)
{
  const Window xid = g_random_int();
  auto fake_window = AddFakeWindowToWM(xid);
  wm->Maximize(xid);

  nux::Geometry expected_geo(fake_window->geo);
  unsigned top_deco = wm->GetWindowDecorationSize(xid, WindowManager::Edge::TOP).height;
  expected_geo.height += top_deco;

  LayoutWindow lwin(xid);
  EXPECT_EQ(lwin.xid, xid);
  EXPECT_EQ(lwin.geo, expected_geo);
  EXPECT_EQ(lwin.decoration_height, top_deco);
  EXPECT_EQ(lwin.selected, false);
  EXPECT_EQ(lwin.aspect_ratio, expected_geo.width / static_cast<float>(expected_geo.height));
}

TEST(TestLayoutWindow, InitializationMinimizedMaximizedWindow)
{
  const Window xid = g_random_int();
  auto fake_window = AddFakeWindowToWM(xid);
  wm->Maximize(xid);
  wm->Minimize(xid);

  LayoutWindow lwin(xid);
  EXPECT_EQ(lwin.xid, xid);
  EXPECT_EQ(lwin.geo, fake_window->geo);
  EXPECT_EQ(lwin.decoration_height, 0);
  EXPECT_EQ(lwin.selected, false);
  EXPECT_EQ(lwin.aspect_ratio, fake_window->geo.width / static_cast<float>(fake_window->geo.height));
}

}
}
}
