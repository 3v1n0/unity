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

#include <vector>

namespace unity
{
namespace ui
{
namespace
{
StandaloneWindowManager* wm = nullptr;

StandaloneWindow::Ptr AddFakeWindowToWM(Window xid, nux::Geometry const& geo = nux::Geometry(1, 2, 30, 40))
{
  const unsigned top_deco = 5;
  auto fake_window = std::make_shared<StandaloneWindow>(xid);
  fake_window->geo = geo;
  fake_window->deco_sizes[unsigned(WindowManager::Edge::TOP)] = nux::Size(geo.width, top_deco);

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
  EXPECT_EQ(lwin.aspect_ratio, fake_window->geo().width / static_cast<float>(fake_window->geo().height));

  wm->ResetStatus();
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
  EXPECT_EQ(lwin.aspect_ratio, fake_window->geo().width / static_cast<float>(fake_window->geo().height));

  wm->ResetStatus();
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

  wm->ResetStatus();
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
  EXPECT_EQ(lwin.aspect_ratio, fake_window->geo().width / static_cast<float>(fake_window->geo().height));

  wm->ResetStatus();
}

struct TestLayoutSystem : testing::Test
{
  TestLayoutSystem()
  {
    Window xid = 1;
    AddFakeWindowToWM(xid, nux::Geometry(4, 5, 500, 600));
    lwindows.push_back(std::make_shared<LayoutWindow>(xid));

    xid = 2;
    AddFakeWindowToWM(xid, nux::Geometry(10, 20, 800, 300));
    lwindows.push_back(std::make_shared<LayoutWindow>(xid));
  }

  ~TestLayoutSystem()
  {
    wm->ResetStatus();
  }

  LayoutSystem ls;
  LayoutWindow::Vector lwindows;
};

TEST_F(TestLayoutSystem, Initialization)
{
  EXPECT_EQ(ls.spacing, 8);
  EXPECT_EQ(ls.max_row_height, 400);
}

TEST_F(TestLayoutSystem, LayoutWindows)
{
  nux::Geometry max_bounds(0, 0, 200, 100);
  nux::Geometry final_bounds;
  ls.LayoutWindows(lwindows, max_bounds, final_bounds);

  nux::Geometry const& win_new_geo1 = lwindows.at(0)->result;
  nux::Geometry const& win_new_geo2 = lwindows.at(1)->result;

  EXPECT_EQ(max_bounds.Intersect(final_bounds), final_bounds);
  EXPECT_NE(lwindows.at(0)->geo, win_new_geo1);
  EXPECT_NE(lwindows.at(1)->geo, win_new_geo1);

  // Computing the area occupied by the grouped windows
  unsigned min_start_x = std::min(win_new_geo1.x, win_new_geo2.x);
  unsigned min_start_y = std::min(win_new_geo1.y, win_new_geo2.y);
  unsigned max_last_x = std::max(win_new_geo1.x + win_new_geo1.width,
                                 win_new_geo2.x + win_new_geo2.width);
  unsigned max_last_y = std::max(win_new_geo1.y + win_new_geo1.height,
                                 win_new_geo2.y + win_new_geo2.height);

  nux::Geometry windows_area(min_start_x, min_start_y, max_last_x - min_start_x, max_last_y - min_start_y);

  EXPECT_EQ(final_bounds.Intersect(windows_area), windows_area);
}

TEST_F(TestLayoutSystem, GetRowSizesEven)
{
  nux::Geometry max_bounds(0, 0, 200, 100);
  nux::Geometry final_bounds;

  Window xid = 3;
  AddFakeWindowToWM(xid, nux::Geometry(4, 5, 200, 200));
  lwindows.push_back(std::make_shared<LayoutWindow>(xid));

  xid = 4;
  AddFakeWindowToWM(xid, nux::Geometry(10, 20, 200, 200));
  lwindows.push_back(std::make_shared<LayoutWindow>(xid));

  ls.LayoutWindows(lwindows, max_bounds, final_bounds);

  std::vector<int> const& row_sizes = ls.GetRowSizes(lwindows, max_bounds);
  EXPECT_EQ(row_sizes.size(), 2);
  EXPECT_EQ(row_sizes[0], 2);
  EXPECT_EQ(row_sizes[1], 2);
}

TEST_F(TestLayoutSystem, GetRowSizesUnEven)
{
  nux::Geometry max_bounds(0, 0, 200, 100);
  nux::Geometry final_bounds;

  Window xid = 3;
  AddFakeWindowToWM(xid, nux::Geometry(4, 5, 200, 200));
  lwindows.push_back(std::make_shared<LayoutWindow>(xid));

  xid = 4;
  AddFakeWindowToWM(xid, nux::Geometry(10, 20, 200, 200));
  lwindows.push_back(std::make_shared<LayoutWindow>(xid));

  xid = 5;
  AddFakeWindowToWM(xid, nux::Geometry(10, 20, 200, 200));
  lwindows.push_back(std::make_shared<LayoutWindow>(xid));

  ls.LayoutWindows(lwindows, max_bounds, final_bounds);

  std::vector<int> const& row_sizes = ls.GetRowSizes(lwindows, max_bounds);
  EXPECT_EQ(row_sizes.size(), 2);
  EXPECT_EQ(row_sizes[0], 2);
  EXPECT_EQ(row_sizes[1], 3);
}

}
}
}
