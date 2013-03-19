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
 *              Andrea Azzarone <andrea.azzarone@canonical.com>
 */

#include <gmock/gmock.h>

#include "launcher/ExpoLauncherIcon.h"
#include "unity-shared/StandaloneWindowManager.h"

using namespace unity;
using namespace unity::launcher;

namespace
{

struct TestExpoLauncherIcon : testing::Test
{
  TestExpoLauncherIcon()
    : wm(dynamic_cast<StandaloneWindowManager*>(&WindowManager::Default()))
  {}

  void TearDown()
  {
    wm->SetViewportSize(2, 2);
  }

  ExpoLauncherIcon icon;
  StandaloneWindowManager* wm;
};

TEST_F(TestExpoLauncherIcon, ActivateToggleExpo)
{
  ASSERT_FALSE(wm->IsExpoActive());

  icon.Activate(ActionArg());
  ASSERT_TRUE(wm->IsExpoActive());

  icon.Activate(ActionArg());
  EXPECT_FALSE(wm->IsExpoActive());
}

TEST_F(TestExpoLauncherIcon, Position)
{
  EXPECT_EQ(icon.position(), AbstractLauncherIcon::Position::FLOATING);
}

TEST_F(TestExpoLauncherIcon, RemoteUri)
{
  EXPECT_EQ(icon.RemoteUri(), "unity://expo-icon");
}

TEST_F(TestExpoLauncherIcon, Icon2x2Layout)
{
  EXPECT_EQ(icon.icon_name, "workspace-switcher-top-left");

  wm->SetCurrentViewport(nux::Point(1, 0));
  wm->screen_viewport_switch_ended.emit();
  EXPECT_EQ(icon.icon_name, "workspace-switcher-right-top");

  wm->SetCurrentViewport(nux::Point(0, 1));
  wm->screen_viewport_switch_ended.emit();
  EXPECT_EQ(icon.icon_name, "workspace-switcher-left-bottom");

  wm->SetCurrentViewport(nux::Point(1, 1));
  wm->screen_viewport_switch_ended.emit();
  EXPECT_EQ(icon.icon_name, "workspace-switcher-right-bottom");

  wm->SetCurrentViewport(nux::Point(0, 0));
  wm->screen_viewport_switch_ended.emit();
  EXPECT_EQ(icon.icon_name, "workspace-switcher-top-left");
}

TEST_F(TestExpoLauncherIcon, Icon2x2Layout_Expo)
{
  EXPECT_EQ(icon.icon_name, "workspace-switcher-top-left");

  wm->SetCurrentViewport(nux::Point(1, 0));
  wm->terminate_expo.emit();
  EXPECT_EQ(icon.icon_name, "workspace-switcher-right-top");

  wm->SetCurrentViewport(nux::Point(0, 1));
  wm->terminate_expo.emit();
  EXPECT_EQ(icon.icon_name, "workspace-switcher-left-bottom");

  wm->SetCurrentViewport(nux::Point(1, 1));
  wm->terminate_expo.emit();
  EXPECT_EQ(icon.icon_name, "workspace-switcher-right-bottom");

  wm->SetCurrentViewport(nux::Point(0, 0));
  wm->terminate_expo.emit();
  EXPECT_EQ(icon.icon_name, "workspace-switcher-top-left");
}

TEST_F(TestExpoLauncherIcon, IconNot2x2Layout)
{
  wm->SetCurrentViewport(nux::Point(1, 0));
  wm->screen_viewport_switch_ended.emit();
  EXPECT_EQ(icon.icon_name, "workspace-switcher-right-top");

  wm->viewport_layout_changed.emit(5, 2);
  EXPECT_EQ(icon.icon_name, "workspace-switcher-top-left");

  wm->SetCurrentViewport(nux::Point(1, 1));
  wm->screen_viewport_switch_ended.emit();
  EXPECT_EQ(icon.icon_name, "workspace-switcher-top-left");
}

TEST_F(TestExpoLauncherIcon, AboutToRemoveDisablesViewport)
{
  wm->SetViewportSize(2, 2);

  icon.AboutToRemove();
  EXPECT_EQ(wm->GetViewportHSize(), 1);
  EXPECT_EQ(wm->GetViewportVSize(), 1);
}

}
