// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 *              Marco Trevisan <marco.trevisan@canonical.com>
 */

#include <list>
#include <gmock/gmock.h>
#include "test_uscreen_mock.h"
using namespace testing;

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>

#include "launcher/DNDCollectionWindow.h"
#include "launcher/MockLauncherIcon.h"
#include "launcher/Launcher.h"
#include "unity-shared/PanelStyle.h"
#include "unity-shared/UnitySettings.h"
#include "unity-shared/IconRenderer.h"
#include "test_utils.h"
using namespace unity;

namespace unity
{
namespace launcher
{
namespace
{

const int STARTING_ANIMATION_DURATION = 150;

class MockMockLauncherIcon : public launcher::MockLauncherIcon
{
public:
  typedef nux::ObjectPtr<MockMockLauncherIcon> Ptr;

  MOCK_METHOD1(ShouldHighlightOnDrag, bool(DndData const&));
};

}

class TestLauncher : public Test
{
public:
  class MockLauncher : public launcher::Launcher
  {
  public:
    MockLauncher(nux::BaseWindow* parent, nux::ObjectPtr<DNDCollectionWindow> const& collection_window)
      : Launcher(parent, collection_window)
    {}

    AbstractLauncherIcon::Ptr MouseIconIntersection(int x, int y)
    {
      for (auto const& icon : *_model)
      {
        auto const& center = icon->GetCenter(monitor());

        if (y > center.y - GetIconSize()/2.0f && y < center.y + GetIconSize()/2.0f)
          return icon;
      }

      return AbstractLauncherIcon::Ptr();
    }

    float IconBackgroundIntensity(AbstractLauncherIcon::Ptr icon, timespec const& current) const
    {
      return Launcher::IconBackgroundIntensity(icon, current);
    }

    void StartIconDrag(AbstractLauncherIcon::Ptr icon)
    {
      Launcher::StartIconDrag(icon);
    }

    void ShowDragWindow()
    {
      Launcher::ShowDragWindow();
    }

    void UpdateDragWindowPosition(int x, int y)
    {
      Launcher::UpdateDragWindowPosition(x, y);
    }

    void HideDragWindow()
    {
      Launcher::HideDragWindow();
    }

    void ResetMouseDragState()
    {
      Launcher::ResetMouseDragState();
    }
  };

  TestLauncher()
    : parent_window_(new nux::BaseWindow("TestLauncherWindow"))
    , dnd_collection_window_(new DNDCollectionWindow)
    , model_(new LauncherModel)
    , options_(new Options)
    , launcher_(new MockLauncher(parent_window_, dnd_collection_window_))
  {
    launcher_->options = options_;
    launcher_->SetModel(model_);
  }

  MockUScreen uscreen;
  nux::BaseWindow* parent_window_;
  nux::ObjectPtr<DNDCollectionWindow> dnd_collection_window_;
  Settings settings;
  panel::Style panel_style;
  LauncherModel::Ptr model_;
  Options::Ptr options_;
  nux::ObjectPtr<MockLauncher> launcher_;
};

TEST_F(TestLauncher, TestQuirksDuringDnd)
{
  MockMockLauncherIcon::Ptr first(new MockMockLauncherIcon);
  model_->AddIcon(first);

  MockMockLauncherIcon::Ptr second(new MockMockLauncherIcon);
  model_->AddIcon(second);

  MockMockLauncherIcon::Ptr third(new MockMockLauncherIcon);
  model_->AddIcon(third);

  EXPECT_CALL(*first, ShouldHighlightOnDrag(_))
      .WillRepeatedly(Return(true));

  EXPECT_CALL(*second, ShouldHighlightOnDrag(_))
      .WillRepeatedly(Return(true));

  EXPECT_CALL(*third, ShouldHighlightOnDrag(_))
      .WillRepeatedly(Return(false));

  std::list<char*> uris;
  dnd_collection_window_->collected.emit(uris);

  Utils::WaitForTimeout(1);

  EXPECT_FALSE(first->GetQuirk(launcher::AbstractLauncherIcon::Quirk::DESAT));
  EXPECT_FALSE(second->GetQuirk(launcher::AbstractLauncherIcon::Quirk::DESAT));
  EXPECT_TRUE(third->GetQuirk(launcher::AbstractLauncherIcon::Quirk::DESAT));
}

TEST_F(TestLauncher, TestMouseWheelScroll)
{
  int initial_scroll_delta;

  launcher_->SetHover(true);
  initial_scroll_delta = launcher_->GetDragDelta();

  // scroll down
  launcher_->RecvMouseWheel(0,0,20,0,0);
  EXPECT_EQ((launcher_->GetDragDelta() - initial_scroll_delta), 25);

  // scroll up
  launcher_->RecvMouseWheel(0,0,-20,0,0);
  EXPECT_EQ(launcher_->GetDragDelta(), initial_scroll_delta);

  launcher_->SetHover(false);
}

TEST_F(TestLauncher, TestIconBackgroundIntensity)
{
  MockMockLauncherIcon::Ptr first(new MockMockLauncherIcon);
  model_->AddIcon(first);

  MockMockLauncherIcon::Ptr second(new MockMockLauncherIcon);
  model_->AddIcon(second);

  MockMockLauncherIcon::Ptr third(new MockMockLauncherIcon);
  model_->AddIcon(third);

  options_->backlight_mode = BACKLIGHT_NORMAL;
  options_->launch_animation = LAUNCH_ANIMATION_PULSE;

  first->SetQuirk(AbstractLauncherIcon::Quirk::RUNNING, true);
  second->SetQuirk(AbstractLauncherIcon::Quirk::RUNNING, true);
  third->SetQuirk(AbstractLauncherIcon::Quirk::RUNNING, false);

  Utils::WaitForTimeoutMSec(STARTING_ANIMATION_DURATION);
  timespec current;
  clock_gettime(CLOCK_MONOTONIC, &current);

  EXPECT_THAT(launcher_->IconBackgroundIntensity(first, current), Gt(0.0f));
  EXPECT_THAT(launcher_->IconBackgroundIntensity(second, current), Gt(0.0f));
  EXPECT_EQ(launcher_->IconBackgroundIntensity(third, current), 0.0f);
}

TEST_F(TestLauncher, DragLauncherIconCancelRestoresIconOrder)
{
  MockMockLauncherIcon::Ptr icon1(new MockMockLauncherIcon);
  MockMockLauncherIcon::Ptr icon2(new MockMockLauncherIcon);
  MockMockLauncherIcon::Ptr icon3(new MockMockLauncherIcon);

  model_->AddIcon(icon1);
  model_->AddIcon(icon2);
  model_->AddIcon(icon3);

  // Set the icon centers
  int icon_size = launcher_->GetIconSize();
  icon1->SetCenter(nux::Point3(icon_size/2.0f, icon_size/2.0f * 1 + 1, 0), launcher_->monitor(), launcher_->GetGeometry());
  icon2->SetCenter(nux::Point3(icon_size/2.0f, icon_size/2.0f * 2 + 1, 0), launcher_->monitor(), launcher_->GetGeometry());
  icon3->SetCenter(nux::Point3(icon_size/2.0f, icon_size/2.0f * 3 + 1, 0), launcher_->monitor(), launcher_->GetGeometry());

  // Start dragging icon2
  launcher_->StartIconDrag(icon2);
  launcher_->ShowDragWindow();

  // Moving icon2 at the end
  auto const& center3 = icon3->GetCenter(launcher_->monitor());
  launcher_->UpdateDragWindowPosition(center3.x, center3.y);

  auto it = model_->begin();
  ASSERT_EQ(*it, icon1); it++;
  ASSERT_EQ(*it, icon3); it++;
  ASSERT_EQ(*it, icon2);

  // Moving icon2 at the begin
  auto const& center1 = icon1->GetCenter(launcher_->monitor());
  launcher_->UpdateDragWindowPosition(center1.x, center1.y);

  it = model_->begin();
  ASSERT_EQ(*it, icon2); it++;
  ASSERT_EQ(*it, icon1); it++;
  ASSERT_EQ(*it, icon3);

  bool model_saved = false;
  model_->saved.connect([&model_saved] { model_saved = true; });

  // Emitting the drag cancel request
  launcher_->GetDraggedIcon()->drag_cancel_request.emit();

  // The icon order should be reset
  it = model_->begin();
  ASSERT_EQ(*it, icon1); it++;
  ASSERT_EQ(*it, icon2); it++;
  ASSERT_EQ(*it, icon3);

  EXPECT_FALSE(model_saved);

  launcher_->HideDragWindow();
}

}
}

