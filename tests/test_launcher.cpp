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

    void EndIconDrag()
    {
      Launcher::EndIconDrag();
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

    int GetDragIconPosition() const
    {
      return _drag_icon_position;
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

  std::vector<MockMockLauncherIcon::Ptr> AddMockIcons(unsigned number)
  {
    std::vector<MockMockLauncherIcon::Ptr> icons;
    int icon_size = launcher_->GetIconSize();
    int monitor = launcher_->monitor();
    auto const& launcher_geo = launcher_->GetGeometry();

    for (unsigned i = 0; i < number; ++i)
    {
      MockMockLauncherIcon::Ptr icon(new MockMockLauncherIcon);
      icon->SetCenter(nux::Point3(icon_size/2.0f, icon_size/2.0f * (i+1) + 1, 0), monitor, launcher_geo);

      icons.push_back(icon);
      model_->AddIcon(icon);
    }

    return icons;
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
  auto const& icons = AddMockIcons(3);
  ASSERT_EQ(icons.size(), 3);

  auto const& icon1 = icons[0];
  auto const& icon2 = icons[1];
  auto const& icon3 = icons[2];

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

  // Let's wait the drag icon animation to be completed
  Utils::WaitForTimeout(1);
  EXPECT_EQ(launcher_->GetDraggedIcon(), nullptr);
}

TEST_F(TestLauncher, DragLauncherIconSavesIconOrderIfPositionHasChanged)
{
  auto const& icons = AddMockIcons(3);
  ASSERT_EQ(icons.size(), 3);

  auto const& icon1 = icons[0];
  auto const& icon2 = icons[1];
  auto const& icon3 = icons[2];

  // Start dragging icon2
  launcher_->StartIconDrag(icon2);
  launcher_->ShowDragWindow();
  ASSERT_EQ(launcher_->GetDragIconPosition(), model_->IconIndex(icon2));

  // Moving icon2 at the end
  auto const& center3 = icon3->GetCenter(launcher_->monitor());
  launcher_->UpdateDragWindowPosition(center3.x, center3.y);

  bool model_saved = false;
  model_->saved.connect([&model_saved] { model_saved = true; });

  ASSERT_NE(launcher_->GetDragIconPosition(), model_->IconIndex(icon2));
  launcher_->EndIconDrag();

  // The icon order should be reset
  auto it = model_->begin();
  ASSERT_EQ(*it, icon1); it++;
  ASSERT_EQ(*it, icon3); it++;
  ASSERT_EQ(*it, icon2);

  EXPECT_TRUE(model_saved);

  // Let's wait the drag icon animation to be completed
  Utils::WaitForTimeout(1);
  EXPECT_EQ(launcher_->GetDraggedIcon(), nullptr);
}

TEST_F(TestLauncher, DragLauncherIconSavesIconOrderIfPositionHasNotChanged)
{
  auto const& icons = AddMockIcons(3);
  ASSERT_EQ(icons.size(), 3);

  auto const& icon1 = icons[0];
  auto const& icon2 = icons[1];
  auto const& icon3 = icons[2];

  // Start dragging icon2
  launcher_->StartIconDrag(icon2);
  launcher_->ShowDragWindow();
  ASSERT_EQ(launcher_->GetDragIconPosition(), model_->IconIndex(icon2));

  // Moving icon2 at the end
  auto center3 = icon3->GetCenter(launcher_->monitor());
  launcher_->UpdateDragWindowPosition(center3.x, center3.y);

  // Swapping the centers
  icon3->SetCenter(icon2->GetCenter(launcher_->monitor()), launcher_->monitor(), launcher_->GetGeometry());
  icon2->SetCenter(center3, launcher_->monitor(), launcher_->GetGeometry());

  // Moving icon2 back to the middle
  center3 = icon3->GetCenter(launcher_->monitor());
  launcher_->UpdateDragWindowPosition(center3.x, center3.y);

  bool model_saved = false;
  model_->saved.connect([&model_saved] { model_saved = true; });

  ASSERT_EQ(launcher_->GetDragIconPosition(), model_->IconIndex(icon2));
  launcher_->EndIconDrag();

  // The icon order should be reset
  auto it = model_->begin();
  ASSERT_EQ(*it, icon1); it++;
  ASSERT_EQ(*it, icon2); it++;
  ASSERT_EQ(*it, icon3);

  EXPECT_FALSE(model_saved);

  // Let's wait the drag icon animation to be completed
  Utils::WaitForTimeout(1);
  EXPECT_EQ(launcher_->GetDraggedIcon(), nullptr);
}

}
}

