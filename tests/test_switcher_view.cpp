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
#include "unity-shared/IconRenderer.h"
#include "unity-shared/UnitySettings.h"

namespace unity
{
using namespace launcher;
namespace switcher
{

struct TestSwitcherView : testing::Test
{
  struct MockSwitcherView : SwitcherView
  {
    using SwitcherView::text_view_;
    using SwitcherView::icon_renderer_;
    using SwitcherView::model_;
  };

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
}

}
}
