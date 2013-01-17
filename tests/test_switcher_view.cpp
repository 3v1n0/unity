/*
 * Copyright 2013 Canonical Ltd.
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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 *
 */

#include <gtest/gtest.h>

#include "SwitcherModel.h"
#include "SwitcherView.h"
#include "MockLauncherIcon.h"
#include "StandaloneWindowManager.h"
#include "unity-shared/IconRenderer.h"
#include "unity-shared/UnitySettings.h"

namespace unity
{
using namespace launcher;
namespace switcher
{
namespace
{
  struct MockMockLauncherIcon : MockLauncherIcon
  {
    WindowList Windows() { return windows_; }
    void AddWindow(Window xid) { windows_.push_back(std::make_shared<MockApplicationWindow>(xid)); }

    WindowList windows_;
  };

  int rand_coord() { return g_random_int_range(1, 1024); }
}

struct TestSwitcherView : testing::Test
{
  TestSwitcherView()
    : WM(dynamic_cast<StandaloneWindowManager*>(&WindowManager::Default()))
  {}

  struct MockSwitcherView : SwitcherView
  {
    using SwitcherView::UpdateRenderTargets;
    using SwitcherView::ResizeRenderTargets;
    using SwitcherView::text_view_;
    using SwitcherView::icon_renderer_;
    using SwitcherView::model_;
  };

  StandaloneWindow::Ptr AddFakeWindowToWM(Window xid)
  {
    const unsigned top_deco = 5;
    auto fake_window = std::make_shared<StandaloneWindow>(xid);
    fake_window->geo = nux::Geometry(rand_coord(), rand_coord(), rand_coord(), rand_coord());
    fake_window->deco_sizes[unsigned(WindowManager::Edge::TOP)] = nux::Size(fake_window->geo.width, top_deco);

    WM->AddStandaloneWindow(fake_window);

    return fake_window;
  }

  StandaloneWindowManager* WM;
  unity::Settings settings;
  MockSwitcherView switcher;
};

TEST_F(TestSwitcherView, Initiate)
{
  EXPECT_FALSE(switcher.render_boxes);
  EXPECT_EQ(switcher.border_size, 50);
  EXPECT_EQ(switcher.flat_spacing, 10);
  EXPECT_EQ(switcher.icon_size, 128);
  EXPECT_EQ(switcher.minimum_spacing, 10);
  EXPECT_EQ(switcher.tile_size, 150);
  EXPECT_EQ(switcher.vertical_size, switcher.tile_size + 80);
  EXPECT_EQ(switcher.text_size, 15);
  EXPECT_EQ(switcher.animation_length, 250);
  EXPECT_EQ(switcher.monitor, -1);
  EXPECT_EQ(switcher.spread_size, 3.5f);
  ASSERT_NE(switcher.text_view_, nullptr);
  ASSERT_NE(switcher.icon_renderer_, nullptr);
  EXPECT_EQ(switcher.icon_renderer_->pip_style, ui::OVER_TILE);
}

TEST_F(TestSwitcherView, SetModel)
{
  SwitcherModel::Applications apps;
  apps.push_back(AbstractLauncherIcon::Ptr(new MockLauncherIcon()));
  apps.push_back(AbstractLauncherIcon::Ptr(new MockLauncherIcon()));
  apps.push_back(AbstractLauncherIcon::Ptr(new MockLauncherIcon()));
  auto model = std::make_shared<SwitcherModel>(apps);

  switcher.SetModel(model);
  ASSERT_EQ(switcher.model_, model);
  ASSERT_EQ(switcher.GetModel(), model);
  EXPECT_FALSE(switcher.model_->selection_changed.empty());
  EXPECT_FALSE(switcher.model_->detail_selection.changed.empty());
  EXPECT_FALSE(switcher.model_->detail_selection_index.changed.empty());
}

TEST_F(TestSwitcherView, ResizeRenderTargets)
{
  MockMockLauncherIcon* app = new MockMockLauncherIcon();

  for (unsigned i = 0; i < 5; ++i)
  {
    Window xid = g_random_int();
    AddFakeWindowToWM(xid);
    app->AddWindow(xid);
  }

  SwitcherModel::Applications apps;
  apps.push_back(AbstractLauncherIcon::Ptr(app));
  switcher.SetModel(std::make_shared<SwitcherModel>(apps));

  for (float progress = 0.1f; progress <= 1.0f; progress += 0.1f)
  {
    auto const& layout_geo = switcher.UpdateRenderTargets(progress);
    std::map<Window, nux::Geometry> old_thumbs;

    for (auto const& win : switcher.ExternalTargets())
      old_thumbs[win->xid] = win->result;

    float to_finish = 1.0f - progress;
    nux::Point layout_abs_center((layout_geo.x + layout_geo.width/2.0f) * to_finish,
                                 (layout_geo.y + layout_geo.height/2.0f) * to_finish);

    switcher.ResizeRenderTargets(layout_geo, progress);

    for (auto const& win : switcher.ExternalTargets())
    {
      auto const& thumb_geo = win->result;
      auto const& old_thumb_geo = old_thumbs[win->xid];

      nux::Geometry expected_geo;
      expected_geo.x = old_thumb_geo.x * progress + layout_abs_center.x;
      expected_geo.y = old_thumb_geo.y * progress + layout_abs_center.y;
      expected_geo.width = old_thumb_geo.width * progress;
      expected_geo.height = old_thumb_geo.height * progress;

      // Like ASSERT_EQ(thumb_geo, expected_geo), but more informative on failure
      ASSERT_EQ(thumb_geo.x, expected_geo.x);
      ASSERT_EQ(thumb_geo.y, expected_geo.y);
      ASSERT_EQ(thumb_geo.width, expected_geo.width);
      ASSERT_EQ(thumb_geo.height, expected_geo.height);
      ASSERT_EQ(thumb_geo, expected_geo);
    }
  }
}

}
}
