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

#include "LauncherIcon.h"
#include "MultiMonitor.h"
#include "UnitySettings.h"

using namespace unity;
using namespace unity::launcher;

namespace
{
struct MockLauncherIcon : LauncherIcon
{
  MockLauncherIcon(IconType type)
    : LauncherIcon(type)
  {}

  virtual nux::BaseTexture* GetTextureForSize(int size) { return nullptr; }
};

struct TestLauncherIcon : testing::Test
{
  TestLauncherIcon()
  	: icon(AbstractLauncherIcon::IconType::APPLICATION)
  {}

  unity::Settings settings;
  MockLauncherIcon icon;
};

TEST_F(TestLauncherIcon, Construction)
{
  EXPECT_EQ(icon.GetIconType(), AbstractLauncherIcon::IconType::APPLICATION);
  EXPECT_EQ(icon.position(), AbstractLauncherIcon::Position::FLOATING);
  EXPECT_EQ(icon.SortPriority(), AbstractLauncherIcon::DefaultPriority(AbstractLauncherIcon::IconType::APPLICATION));
  EXPECT_FALSE(icon.IsSticky());
  EXPECT_FALSE(icon.IsVisible());

  for (unsigned i = 0; i < unsigned(AbstractLauncherIcon::Quirk::LAST); ++i)
    ASSERT_FALSE(icon.GetQuirk(static_cast<AbstractLauncherIcon::Quirk>(i)));

  for (unsigned i = 0; i < monitors::MAX; ++i)
    ASSERT_TRUE(icon.IsVisibleOnMonitor(i));
}

TEST_F(TestLauncherIcon, Visibility)
{
  ASSERT_FALSE(icon.GetQuirk(AbstractLauncherIcon::Quirk::VISIBLE));
  ASSERT_FALSE(icon.IsVisible());

  icon.SetQuirk(AbstractLauncherIcon::Quirk::VISIBLE, true);
  ASSERT_TRUE(icon.GetQuirk(AbstractLauncherIcon::Quirk::VISIBLE));
  EXPECT_TRUE(icon.IsVisible());

  icon.SetQuirk(AbstractLauncherIcon::Quirk::VISIBLE, false);
  ASSERT_FALSE(icon.GetQuirk(AbstractLauncherIcon::Quirk::VISIBLE));
  EXPECT_FALSE(icon.IsVisible());
}

TEST_F(TestLauncherIcon, Stick)
{
  bool saved = false;
  icon.position_saved.connect([&saved] {saved = true;});

  icon.Stick(false);
  EXPECT_TRUE(icon.IsSticky());
  EXPECT_TRUE(icon.IsVisible());
  EXPECT_FALSE(saved);

  icon.Stick(true);
  EXPECT_FALSE(saved);
}

TEST_F(TestLauncherIcon, StickAndSave)
{
  bool saved = false;
  icon.position_saved.connect([&saved] {saved = true;});

  icon.Stick(true);
  EXPECT_TRUE(icon.IsSticky());
  EXPECT_TRUE(icon.IsVisible());
  EXPECT_TRUE(saved);
}

TEST_F(TestLauncherIcon, Unstick)
{
  bool forgot = false;
  icon.position_forgot.connect([&forgot] {forgot = true;});

  icon.Stick(false);
  ASSERT_TRUE(icon.IsSticky());
  ASSERT_TRUE(icon.IsVisible());

  icon.UnStick();
  EXPECT_FALSE(icon.IsSticky());
  EXPECT_FALSE(icon.IsVisible());
  EXPECT_TRUE(forgot);
}

TEST_F(TestLauncherIcon, TooltipVisibilityConstruction)
{
  EXPECT_TRUE(icon.tooltip_text().empty());
  EXPECT_TRUE(icon.tooltip_enabled());
}

TEST_F(TestLauncherIcon, TooltipVisibilityValid)
{
  bool tooltip_shown = false;
  icon.tooltip_text = "Unity";
  icon.tooltip_visible.connect([&tooltip_shown] (nux::ObjectPtr<nux::View>) {tooltip_shown = true;});
  icon.ShowTooltip();
  EXPECT_TRUE(tooltip_shown);
}

TEST_F(TestLauncherIcon, TooltipVisibilityEmpty)
{
  bool tooltip_shown = false;
  icon.tooltip_text = "";
  icon.tooltip_visible.connect([&tooltip_shown] (nux::ObjectPtr<nux::View>) {tooltip_shown = true;});
  icon.ShowTooltip();
  EXPECT_FALSE(tooltip_shown);
}

TEST_F(TestLauncherIcon, TooltipVisibilityDisabled)
{
  bool tooltip_shown = false;
  icon.tooltip_text = "Unity";
  icon.tooltip_enabled = false;
  icon.tooltip_visible.connect([&tooltip_shown] (nux::ObjectPtr<nux::View>) {tooltip_shown = true;});
  icon.ShowTooltip();
  EXPECT_FALSE(tooltip_shown);
}

}
