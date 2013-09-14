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

#include "launcher/MockLauncherIcon.h"
#include "launcher/Launcher.h"
#include "unity-shared/PanelStyle.h"
#include "unity-shared/UnitySettings.h"
#include "unity-shared/IconRenderer.h"
#include "test_utils.h"

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
  MockMockLauncherIcon(IconType type = IconType::APPLICATION)
    : MockLauncherIcon(type)
  {}

  MOCK_METHOD1(ShouldHighlightOnDrag, bool(DndData const&));
  MOCK_METHOD1(Stick, void(bool));
  MOCK_METHOD2(PerformScroll, void(ScrollDirection, Time));
};

}

class TestLauncher : public Test
{
public:
  class MockLauncher : public Launcher
  {
  public:
    MockLauncher(MockableBaseWindow* parent)
      : Launcher(parent)
    {}

    void SetExternalDragState()
    {
      SetActionState(Launcher::LauncherActionState::ACTION_DRAG_EXTERNAL);
    }

    bool IsExternalDragState()
    {
      return GetActionState() == Launcher::LauncherActionState::ACTION_DRAG_EXTERNAL;
    }

    AbstractLauncherIcon::Ptr MouseIconIntersection(int x, int y) const
    {
      for (auto const& icon : *_model)
      {
        auto const& center = icon->GetCenter(monitor());

        if (y > center.y - GetIconSize()/2.0f && y < center.y + GetIconSize()/2.0f)
          return icon;
      }

      return AbstractLauncherIcon::Ptr();
    }

    using Launcher::IconBackgroundIntensity;
    using Launcher::StartIconDrag;
    using Launcher::ShowDragWindow;
    using Launcher::EndIconDrag;
    using Launcher::UpdateDragWindowPosition;
    using Launcher::HideDragWindow;
    using Launcher::ResetMouseDragState;
    using Launcher::DndIsSpecialRequest;
    using Launcher::ProcessDndEnter;
    using Launcher::ProcessDndLeave;
    using Launcher::ProcessDndMove;
    using Launcher::ProcessDndDrop;
    using Launcher::_drag_icon_position;
    using Launcher::_icon_under_mouse;
    using Launcher::IconStartingBlinkValue;
    using Launcher::IconStartingPulseValue;
    using Launcher::HandleBarrierEvent;
    using Launcher::SetHidden;
    using Launcher::HandleUrgentIcon;
    using Launcher::SetUrgentTimer;
    using Launcher::_urgent_timer_running;
    using Launcher::_urgent_finished_time;
    using Launcher::_urgent_wiggle_time;

    void FakeProcessDndMove(int x, int y, std::list<std::string> uris)
    {
      _dnd_data.Reset();

      std::string data_uri;
      for (std::string const& uri : uris)
        data_uri += uri+"\r\n";

      _dnd_data.Fill(data_uri.c_str());

      if (std::find_if(_dnd_data.Uris().begin(), _dnd_data.Uris().end(), [this] (std::string const& uri)
                       {return DndIsSpecialRequest(uri);}) != _dnd_data.Uris().end())
      {
        _steal_drag = true;
      }

      _dnd_hovered_icon = MouseIconIntersection(x, y);
    }
  };

  TestLauncher()
    : parent_window_(new MockableBaseWindow("TestLauncherWindow"))
    , model_(new LauncherModel)
    , options_(new Options)
    , launcher_(new MockLauncher(parent_window_.GetPointer()))
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
    int model_pre_size = model_->Size();

    for (unsigned i = 0; i < number; ++i)
    {
      MockMockLauncherIcon::Ptr icon(new MockMockLauncherIcon);
      icon->SetCenter(nux::Point3(icon_size/2.0f, icon_size/2.0f * (i+1) + 1, 0), monitor, launcher_geo);

      icons.push_back(icon);
      model_->AddIcon(icon);
    }

    EXPECT_EQ(icons.size(), number);
    EXPECT_EQ(model_pre_size + number, number);

    return icons;
  }

  MockUScreen uscreen;
  nux::ObjectPtr<MockableBaseWindow> parent_window_;
  Settings settings;
  panel::Style panel_style;
  LauncherModel::Ptr model_;
  Options::Ptr options_;
  nux::ObjectPtr<MockLauncher> launcher_;
};

struct TestWindowCompositor
{
  static void SetMousePosition(int x, int y)
  {
    nux::GetWindowCompositor()._mouse_position = nux::Point(x, y);
  }
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

  launcher_->DndStarted("");
  Utils::WaitPendingEvents();

  EXPECT_FALSE(first->GetQuirk(launcher::AbstractLauncherIcon::Quirk::DESAT));
  EXPECT_FALSE(second->GetQuirk(launcher::AbstractLauncherIcon::Quirk::DESAT));
  EXPECT_TRUE(third->GetQuirk(launcher::AbstractLauncherIcon::Quirk::DESAT));
}

TEST_F(TestLauncher, TestMouseWheelScroll)
{
  MockMockLauncherIcon::Ptr icon(new MockMockLauncherIcon);
  model_->AddIcon(icon);

  launcher_->SetHover(true);
  launcher_->_icon_under_mouse = icon;

  unsigned long key_flags = 0; 

  EXPECT_CALL(*icon, PerformScroll(AbstractLauncherIcon::ScrollDirection::UP, _));
  launcher_->RecvMouseWheel(0, 0, 20, 0, key_flags);

  EXPECT_CALL(*icon, PerformScroll(AbstractLauncherIcon::ScrollDirection::DOWN, _));
  launcher_->RecvMouseWheel(0, 0, -20, 0, key_flags);

  launcher_->SetHover(false);
}

TEST_F(TestLauncher, TestMouseWheelScrollAltPressed)
{
  int initial_scroll_delta;

  launcher_->SetHover(true);
  initial_scroll_delta = launcher_->GetDragDelta();

  unsigned long key_flags = 0; 

  launcher_->RecvMouseWheel(0, 0, 20, 0, key_flags);
  EXPECT_EQ((launcher_->GetDragDelta()), initial_scroll_delta);

  key_flags |= nux::NUX_STATE_ALT;

  // scroll down 
  launcher_->RecvMouseWheel(0, 0, 20, 0, key_flags);
  EXPECT_EQ((launcher_->GetDragDelta() - initial_scroll_delta), 25);

  // scroll up - alt pressed
  launcher_->RecvMouseWheel(0, 0, -20, 0, key_flags);
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
  Utils::WaitPendingEvents();
  EXPECT_EQ(launcher_->GetDraggedIcon(), nullptr);
}

TEST_F(TestLauncher, DragLauncherIconSavesIconOrderIfPositionHasChanged)
{
  auto const& icons = AddMockIcons(3);

  auto const& icon1 = icons[0];
  auto const& icon2 = icons[1];
  auto const& icon3 = icons[2];

  // Start dragging icon2
  launcher_->StartIconDrag(icon2);
  launcher_->ShowDragWindow();
  ASSERT_EQ(launcher_->_drag_icon_position, model_->IconIndex(icon2));

  // Moving icon2 at the end
  auto const& center3 = icon3->GetCenter(launcher_->monitor());
  launcher_->UpdateDragWindowPosition(center3.x, center3.y);

  EXPECT_CALL(*icon2, Stick(true));

  ASSERT_NE(launcher_->_drag_icon_position, model_->IconIndex(icon2));
  launcher_->EndIconDrag();

  // The icon order should be reset
  auto it = model_->begin();
  ASSERT_EQ(*it, icon1); it++;
  ASSERT_EQ(*it, icon3); it++;
  ASSERT_EQ(*it, icon2);

  // Let's wait the drag icon animation to be completed
  Utils::WaitUntilMSec([this] { return launcher_->GetDraggedIcon(); }, false, 2000);
  EXPECT_EQ(launcher_->GetDraggedIcon(), nullptr);
}

TEST_F(TestLauncher, DragLauncherIconSavesIconOrderIfPositionHasNotChanged)
{
  auto const& icons = AddMockIcons(3);

  auto const& icon1 = icons[0];
  auto const& icon2 = icons[1];
  auto const& icon3 = icons[2];

  // Start dragging icon2
  launcher_->StartIconDrag(icon2);
  launcher_->ShowDragWindow();
  ASSERT_EQ(launcher_->_drag_icon_position, model_->IconIndex(icon2));

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

  ASSERT_EQ(launcher_->_drag_icon_position, model_->IconIndex(icon2));
  launcher_->EndIconDrag();

  // The icon order should be reset
  auto it = model_->begin();
  ASSERT_EQ(*it, icon1); it++;
  ASSERT_EQ(*it, icon2); it++;
  ASSERT_EQ(*it, icon3);

  EXPECT_FALSE(model_saved);

  // Let's wait the drag icon animation to be completed
  Utils::WaitUntilMSec([this] { return launcher_->GetDraggedIcon(); }, false, 2000);
  EXPECT_EQ(launcher_->GetDraggedIcon(), nullptr);
}

TEST_F(TestLauncher, DragLauncherIconSticksApplicationIcon)
{
  auto const& icons = AddMockIcons(1);

  MockMockLauncherIcon::Ptr app(new MockMockLauncherIcon(AbstractLauncherIcon::IconType::APPLICATION));
  model_->AddIcon(app);

  // Start dragging app icon
  launcher_->StartIconDrag(app);
  launcher_->ShowDragWindow();

  // Moving app icon to the beginning
  auto const& center = icons[0]->GetCenter(launcher_->monitor());
  launcher_->UpdateDragWindowPosition(center.x, center.y);

  EXPECT_CALL(*app, Stick(true));
  launcher_->EndIconDrag();
}

TEST_F(TestLauncher, DragLauncherIconSticksDeviceIcon)
{
  auto const& icons = AddMockIcons(1);

  MockMockLauncherIcon::Ptr device(new MockMockLauncherIcon(AbstractLauncherIcon::IconType::DEVICE));
  model_->AddIcon(device);

  // Start dragging device icon
  launcher_->StartIconDrag(device);
  launcher_->ShowDragWindow();

  // Moving device icon to the beginning
  auto const& center = icons[0]->GetCenter(launcher_->monitor());
  launcher_->UpdateDragWindowPosition(center.x, center.y);

  EXPECT_CALL(*device, Stick(true));
  launcher_->EndIconDrag();
}

TEST_F(TestLauncher, DragLauncherIconHidesOverLauncherEmitsMouseEnter)
{
  bool mouse_entered = false;

  launcher_->mouse_enter.connect([&mouse_entered] (int x, int y, unsigned long, unsigned long) {
    mouse_entered = true;
    EXPECT_EQ(x, 1);
    EXPECT_EQ(y, 2);
  });

  auto const& abs_geo = launcher_->GetAbsoluteGeometry();
  TestWindowCompositor::SetMousePosition(abs_geo.x + 1, abs_geo.y + 2);
  launcher_->HideDragWindow();
  EXPECT_TRUE(mouse_entered);
}

TEST_F(TestLauncher, DragLauncherIconHidesOutsideLauncherEmitsMouseEnter)
{
  bool mouse_entered = false;
  launcher_->mouse_enter.connect([&mouse_entered] (int, int, unsigned long, unsigned long)
                                 { mouse_entered = true; });

  auto const& abs_geo = launcher_->GetAbsoluteGeometry();
  TestWindowCompositor::SetMousePosition(abs_geo.x - 1, abs_geo.y - 2);
  launcher_->HideDragWindow();
  EXPECT_FALSE(mouse_entered);
}

TEST_F(TestLauncher, EdgeReleasesDuringDnd)
{
  auto barrier = std::make_shared<ui::PointerBarrierWrapper>();
  auto event = std::make_shared<ui::BarrierEvent>(0, 0, 0, 100);

  launcher_->DndStarted("");

  EXPECT_EQ(launcher_->HandleBarrierEvent(barrier.get(), event),
            ui::EdgeBarrierSubscriber::Result::NEEDS_RELEASE);
}

TEST_F(TestLauncher, EdgeBarriersIgnoreEvents)
{
  auto const& launcher_geo = launcher_->GetAbsoluteGeometry();
  auto barrier = std::make_shared<ui::PointerBarrierWrapper>();
  auto event = std::make_shared<ui::BarrierEvent>(0, 0, 0, 100);
  launcher_->SetHidden(true);

  event->x = launcher_geo.x-1;
  event->y = launcher_geo.y;
  EXPECT_EQ(launcher_->HandleBarrierEvent(barrier.get(), event),
            ui::EdgeBarrierSubscriber::Result::IGNORED);

  event->x = launcher_geo.x+launcher_geo.width+1;
  event->y = launcher_geo.y;
  EXPECT_EQ(launcher_->HandleBarrierEvent(barrier.get(), event),
            ui::EdgeBarrierSubscriber::Result::IGNORED);

  options_->reveal_trigger = RevealTrigger::EDGE;
  event->x = launcher_geo.x+launcher_geo.width/2;
  event->y = launcher_geo.y - 1;
  EXPECT_EQ(launcher_->HandleBarrierEvent(barrier.get(), event),
            ui::EdgeBarrierSubscriber::Result::IGNORED);

  options_->reveal_trigger = RevealTrigger::CORNER;
  event->x = launcher_geo.x+launcher_geo.width/2;
  event->y = launcher_geo.y;
  EXPECT_EQ(launcher_->HandleBarrierEvent(barrier.get(), event),
            ui::EdgeBarrierSubscriber::Result::IGNORED);
}

TEST_F(TestLauncher, EdgeBarriersHandlesEvent)
{
  auto const& launcher_geo = launcher_->GetAbsoluteGeometry();
  auto barrier = std::make_shared<ui::PointerBarrierWrapper>();
  auto event = std::make_shared<ui::BarrierEvent>(0, 0, 0, 100);
  launcher_->SetHidden(true);

  options_->reveal_trigger = RevealTrigger::EDGE;

  for (int x = launcher_geo.x; x < launcher_geo.x+launcher_geo.width; ++x)
  {
    for (int y = launcher_geo.y; y < launcher_geo.y+launcher_geo.height; ++y)
    {
      event->x = x;
      event->y = y;
      ASSERT_EQ(launcher_->HandleBarrierEvent(barrier.get(), event),
                ui::EdgeBarrierSubscriber::Result::HANDLED);
    }
  }

  options_->reveal_trigger = RevealTrigger::CORNER;

  for (int x = launcher_geo.x; x < launcher_geo.x+launcher_geo.width; ++x)
  {
    for (int y = launcher_geo.y-10; y < launcher_geo.y; ++y)
    {
      event->x = x;
      event->y = y;
      ASSERT_EQ(launcher_->HandleBarrierEvent(barrier.get(), event),
                ui::EdgeBarrierSubscriber::Result::HANDLED);
    }
  }
}

TEST_F(TestLauncher, DndIsSpecialRequest)
{
  EXPECT_TRUE(launcher_->DndIsSpecialRequest("MyFile.desktop"));
  EXPECT_TRUE(launcher_->DndIsSpecialRequest("/full/path/to/MyFile.desktop"));
  EXPECT_TRUE(launcher_->DndIsSpecialRequest("application://MyFile.desktop"));
  EXPECT_TRUE(launcher_->DndIsSpecialRequest("file://MyFile.desktop"));
  EXPECT_TRUE(launcher_->DndIsSpecialRequest("file://full/path/to/MyFile.desktop"));
  EXPECT_TRUE(launcher_->DndIsSpecialRequest("device://uuuid"));

  EXPECT_FALSE(launcher_->DndIsSpecialRequest("MyFile.txt"));
  EXPECT_FALSE(launcher_->DndIsSpecialRequest("/full/path/to/MyFile.txt"));
  EXPECT_FALSE(launcher_->DndIsSpecialRequest("file://full/path/to/MyFile.txt"));
}

TEST_F(TestLauncher, AddRequestSignal)
{
  auto const& icons = AddMockIcons(1);
  auto const& center = icons[0]->GetCenter(launcher_->monitor());
  launcher_->ProcessDndEnter();
  launcher_->FakeProcessDndMove(center.x, center.y, {"application://MyFile.desktop"});

  bool add_request = false;
  launcher_->add_request.connect([&] (std::string const& uri, AbstractLauncherIcon::Ptr const& drop_icon) {
    EXPECT_EQ(drop_icon, icons[0]);
    EXPECT_EQ(uri, "application://MyFile.desktop");
    add_request = true;
  });

  launcher_->ProcessDndDrop(center.x, center.y);
  launcher_->ProcessDndLeave();

  EXPECT_TRUE(add_request);
}

TEST_F(TestLauncher, IconStartingPulseValue)
{  
  struct timespec current;
  clock_gettime(CLOCK_MONOTONIC, &current);
  MockMockLauncherIcon::Ptr icon(new MockMockLauncherIcon);

  icon->SetQuirk(AbstractLauncherIcon::Quirk::STARTING, true);

  // Pulse value should start at 0.
  EXPECT_EQ(launcher_->IconStartingPulseValue(icon, current), 0.0);
}

TEST_F(TestLauncher, IconStartingBlinkValue)
{  
  struct timespec current;
  clock_gettime(CLOCK_MONOTONIC, &current);
  MockMockLauncherIcon::Ptr icon(new MockMockLauncherIcon);

  icon->SetQuirk(AbstractLauncherIcon::Quirk::STARTING, true);

  // Pulse value should start at 0.
  EXPECT_EQ(launcher_->IconStartingBlinkValue(icon, current), 0.0);
}

TEST_F(TestLauncher, HighlightingEmptyUrisOnDragMoveIsIgnored)
{
  MockMockLauncherIcon::Ptr first(new MockMockLauncherIcon);
  model_->AddIcon(first);

  EXPECT_CALL(*first, ShouldHighlightOnDrag(_)).Times(0);
  launcher_->ProcessDndMove(0,0,{});
}

TEST_F(TestLauncher, UrgentIconWiggleTimerStart)
{
  MockMockLauncherIcon::Ptr icon(new MockMockLauncherIcon);
  timespec current;

  launcher_->SetHidden(true);
  icon->SetQuirk(AbstractLauncherIcon::Quirk::URGENT, true);

  clock_gettime(CLOCK_MONOTONIC, &current);

  ASSERT_FALSE(launcher_->_urgent_timer_running);

  launcher_->HandleUrgentIcon(icon, current);

  EXPECT_TRUE(launcher_->_urgent_timer_running);
}

TEST_F(TestLauncher, UrgentIconWiggleTimerTimeout)
{
  MockMockLauncherIcon::Ptr icon(new MockMockLauncherIcon);

  model_->AddIcon(icon);
  launcher_->SetHidden(true);
  icon->SetQuirk(AbstractLauncherIcon::Quirk::URGENT, true);

  ASSERT_EQ(launcher_->_urgent_wiggle_time, 0);
  ASSERT_EQ(launcher_->_urgent_finished_time.tv_sec, 0);
  ASSERT_EQ(launcher_->_urgent_finished_time.tv_nsec, 0);
  
  launcher_->SetUrgentTimer(1);

  // Make sure the Urgent Timer expires before checking
  Utils::WaitForTimeout(2);
  
  EXPECT_THAT(launcher_->_urgent_wiggle_time, Gt(0));
  EXPECT_THAT(launcher_->_urgent_finished_time.tv_sec, Gt(0));
  EXPECT_THAT(launcher_->_urgent_finished_time.tv_nsec, Gt(0));
}

TEST_F(TestLauncher, WiggleUrgentIconAfterLauncherIsRevealed)
{
  MockMockLauncherIcon::Ptr icon(new MockMockLauncherIcon);
  timespec current;

  model_->AddIcon(icon);
  launcher_->SetHidden(true);
  icon->SetQuirk(AbstractLauncherIcon::Quirk::URGENT, true);

  // Wait a bit to simulate the icon's initial wiggle
  Utils::WaitForTimeout(1);

  clock_gettime(CLOCK_MONOTONIC, &current);
  launcher_->HandleUrgentIcon(icon, current);

  ASSERT_EQ(launcher_->_urgent_finished_time.tv_sec, 0);
  ASSERT_EQ(launcher_->_urgent_finished_time.tv_nsec, 0);

  launcher_->SetHidden(false);

  clock_gettime(CLOCK_MONOTONIC, &current);
  launcher_->HandleUrgentIcon(icon, current);

  EXPECT_THAT(launcher_->_urgent_finished_time.tv_sec, Gt(0));
  EXPECT_THAT(launcher_->_urgent_finished_time.tv_nsec, Gt(0));  
}  

}
}

