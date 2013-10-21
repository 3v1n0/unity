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
#include "test_standalone_wm.h"
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
  typedef testing::NiceMock<MockMockLauncherIcon> Nice;

  MockMockLauncherIcon(IconType type = IconType::APPLICATION)
    : MockLauncherIcon(type)
  {
    ON_CALL(*this, SetQuirk(_, _, _)).WillByDefault(Invoke([this] (ApplicationLauncherIcon::Quirk q, bool v, int m) { MockLauncherIcon::SetQuirk(q, v, m); }));
    ON_CALL(*this, SetQuirk(_, _)).WillByDefault(Invoke([this] (ApplicationLauncherIcon::Quirk q, bool v) { MockLauncherIcon::SetQuirk(q, v); }));
    ON_CALL(*this, SkipQuirkAnimation(_, _)).WillByDefault(Invoke([this] (ApplicationLauncherIcon::Quirk q, int m) { MockLauncherIcon::SkipQuirkAnimation(q, m); }));
  }

  MOCK_METHOD1(ShouldHighlightOnDrag, bool(DndData const&));
  MOCK_METHOD1(Stick, void(bool));
  MOCK_METHOD2(PerformScroll, void(ScrollDirection, Time));
  MOCK_METHOD0(HideTooltip, void());
  MOCK_METHOD3(SetQuirk, void(ApplicationLauncherIcon::Quirk, bool, int));
  MOCK_METHOD2(SetQuirk, void(ApplicationLauncherIcon::Quirk, bool));
  MOCK_METHOD2(SkipQuirkAnimation, void(ApplicationLauncherIcon::Quirk, int));
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

    bool IsActionStateDragCancelled()
    {
      return GetActionState() == Launcher::LauncherActionState::ACTION_DRAG_ICON_CANCELLED;
    }

    AbstractLauncherIcon::Ptr MouseIconIntersection(int x, int y) const
    {
      for (auto const& icon : *model_)
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
    using Launcher::drag_icon_position_;
    using Launcher::icon_under_mouse_;
    using Launcher::IconStartingBlinkValue;
    using Launcher::IconStartingPulseValue;
    using Launcher::HandleBarrierEvent;
    using Launcher::SetHidden;
    using Launcher::HandleUrgentIcon;
    using Launcher::SetUrgentTimer;
    using Launcher::SetIconUnderMouse;
    using Launcher::OnUrgentTimeout;
    using Launcher::IsOverlayOpen;
    using Launcher::sources_;
    using Launcher::animating_urgent_icons_;
    using Launcher::urgent_animation_period_;

    void FakeProcessDndMove(int x, int y, std::list<std::string> uris)
    {
      dnd_data_.Reset();

      std::string data_uri;
      for (std::string const& uri : uris)
        data_uri += uri+"\r\n";

      dnd_data_.Fill(data_uri.c_str());

      if (std::find_if(dnd_data_.Uris().begin(), dnd_data_.Uris().end(), [this] (std::string const& uri)
                       {return DndIsSpecialRequest(uri);}) != dnd_data_.Uris().end())
      {
        steal_drag_ = true;
      }

      dnd_hovered_icon_ = MouseIconIntersection(x, y);
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
      MockMockLauncherIcon::Ptr icon(new MockMockLauncherIcon::Nice);
      icon->SetCenter(nux::Point3(launcher_geo.x + icon_size/2.0f, launcher_geo.y + icon_size/2.0f * (i+1) + 1, 0), monitor);

      icons.push_back(icon);
      model_->AddIcon(icon);
    }

    EXPECT_EQ(icons.size(), number);
    EXPECT_EQ(model_pre_size + number, number);

    return icons;
  }

  MockUScreen uscreen;
  testwrapper::StandaloneWM WM;
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
  MockMockLauncherIcon::Ptr first(new MockMockLauncherIcon::Nice);
  model_->AddIcon(first);

  MockMockLauncherIcon::Ptr second(new MockMockLauncherIcon::Nice);
  model_->AddIcon(second);

  MockMockLauncherIcon::Ptr third(new MockMockLauncherIcon::Nice);
  model_->AddIcon(third);

  EXPECT_CALL(*first, ShouldHighlightOnDrag(_))
      .WillRepeatedly(Return(true));

  EXPECT_CALL(*second, ShouldHighlightOnDrag(_))
      .WillRepeatedly(Return(true));

  EXPECT_CALL(*third, ShouldHighlightOnDrag(_))
      .WillRepeatedly(Return(false));

  launcher_->DndStarted("");
  Utils::WaitPendingEvents();

  EXPECT_FALSE(first->GetQuirk(launcher::AbstractLauncherIcon::Quirk::DESAT, launcher_->monitor()));
  EXPECT_FALSE(second->GetQuirk(launcher::AbstractLauncherIcon::Quirk::DESAT, launcher_->monitor()));
  EXPECT_TRUE(third->GetQuirk(launcher::AbstractLauncherIcon::Quirk::DESAT, launcher_->monitor()));
}

TEST_F(TestLauncher, TestMouseWheelScroll)
{
  MockMockLauncherIcon::Ptr icon(new MockMockLauncherIcon::Nice);
  model_->AddIcon(icon);

  launcher_->SetHover(true);
  launcher_->icon_under_mouse_ = icon;

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
  MockMockLauncherIcon::Ptr first(new MockMockLauncherIcon::Nice);
  model_->AddIcon(first);

  MockMockLauncherIcon::Ptr second(new MockMockLauncherIcon::Nice);
  model_->AddIcon(second);

  MockMockLauncherIcon::Ptr third(new MockMockLauncherIcon::Nice);
  model_->AddIcon(third);

  options_->backlight_mode = BACKLIGHT_NORMAL;
  options_->launch_animation = LAUNCH_ANIMATION_PULSE;

  first->SetQuirk(AbstractLauncherIcon::Quirk::RUNNING, true);
  second->SetQuirk(AbstractLauncherIcon::Quirk::RUNNING, true);
  third->SetQuirk(AbstractLauncherIcon::Quirk::RUNNING, false);

  Utils::WaitForTimeoutMSec(STARTING_ANIMATION_DURATION);

  EXPECT_THAT(launcher_->IconBackgroundIntensity(first), Gt(0.0f));
  EXPECT_THAT(launcher_->IconBackgroundIntensity(second), Gt(0.0f));
  EXPECT_EQ(launcher_->IconBackgroundIntensity(third), 0.0f);
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
  EXPECT_TRUE(launcher_->IsActionStateDragCancelled());

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
  ASSERT_EQ(launcher_->drag_icon_position_, model_->IconIndex(icon2));

  // Moving icon2 at the end
  auto const& center3 = icon3->GetCenter(launcher_->monitor());
  launcher_->UpdateDragWindowPosition(center3.x, center3.y);

  EXPECT_CALL(*icon2, Stick(true));

  ASSERT_NE(launcher_->drag_icon_position_, model_->IconIndex(icon2));
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
  ASSERT_EQ(launcher_->drag_icon_position_, model_->IconIndex(icon2));

  // Moving icon2 at the end
  auto center3 = icon3->GetCenter(launcher_->monitor());
  launcher_->UpdateDragWindowPosition(center3.x, center3.y);

  // Swapping the centers
  icon3->SetCenter(icon2->GetCenter(launcher_->monitor()), launcher_->monitor());
  icon2->SetCenter(center3, launcher_->monitor());

  // Moving icon2 back to the middle
  center3 = icon3->GetCenter(launcher_->monitor());
  launcher_->UpdateDragWindowPosition(center3.x, center3.y);

  bool model_saved = false;
  model_->saved.connect([&model_saved] { model_saved = true; });

  ASSERT_EQ(launcher_->drag_icon_position_, model_->IconIndex(icon2));
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
  MockMockLauncherIcon::Ptr icon(new MockMockLauncherIcon::Nice);

  icon->SetQuirk(AbstractLauncherIcon::Quirk::STARTING, true);

  // Pulse value should start at 1.
  EXPECT_FLOAT_EQ(launcher_->IconStartingPulseValue(icon), 1.0);
}

TEST_F(TestLauncher, IconStartingBlinkValue)
{
  MockMockLauncherIcon::Ptr icon(new MockMockLauncherIcon::Nice);

  icon->SetQuirk(AbstractLauncherIcon::Quirk::STARTING, true);

  // Pulse value should start at 0.
  EXPECT_FLOAT_EQ(launcher_->IconStartingBlinkValue(icon), 1.0);
}

TEST_F(TestLauncher, HighlightingEmptyUrisOnDragMoveIsIgnored)
{
  MockMockLauncherIcon::Ptr first(new MockMockLauncherIcon::Nice);
  model_->AddIcon(first);

  EXPECT_CALL(*first, ShouldHighlightOnDrag(_)).Times(0);
  launcher_->ProcessDndMove(0,0,{});
}

TEST_F(TestLauncher, UrgentIconTimerStart)
{
  auto icon = AddMockIcons(1).front();
  launcher_->SetHidden(true);
  icon->SetQuirk(AbstractLauncherIcon::Quirk::URGENT, true);
  ASSERT_THAT(launcher_->sources_.GetSource("urgent-timeout"), IsNull());
  ASSERT_TRUE(launcher_->animating_urgent_icons_.empty());

  launcher_->HandleUrgentIcon(icon);
  EXPECT_THAT(launcher_->sources_.GetSource("urgent-timeout"), NotNull());
  ASSERT_EQ(std::set<AbstractLauncherIcon::Ptr>({icon}), launcher_->animating_urgent_icons_);
}

TEST_F(TestLauncher, UrgentIconSaved)
{
  auto icon = AddMockIcons(1).front();
  launcher_->SetHidden(true);
  icon->SetQuirk(AbstractLauncherIcon::Quirk::URGENT, true);
  ASSERT_TRUE(launcher_->animating_urgent_icons_.empty());

  launcher_->HandleUrgentIcon(icon);
  ASSERT_EQ(std::set<AbstractLauncherIcon::Ptr>({icon}), launcher_->animating_urgent_icons_);
}

TEST_F(TestLauncher, UrgentIconIsHandled)
{
  auto icon = AddMockIcons(1).front();
  launcher_->SetHidden(true);
  icon->SetQuirk(AbstractLauncherIcon::Quirk::URGENT, true);
  ASSERT_TRUE(launcher_->animating_urgent_icons_.empty());

  launcher_->HandleUrgentIcon(icon);
  ASSERT_EQ(std::set<AbstractLauncherIcon::Ptr>({icon}), launcher_->animating_urgent_icons_);
}

TEST_F(TestLauncher, UrgentIconsUnhandling)
{
  auto icons = AddMockIcons(2);
  launcher_->SetHidden(true);

  for (auto const& icon : icons)
  {
    icon->SetQuirk(AbstractLauncherIcon::Quirk::URGENT, true);
    launcher_->HandleUrgentIcon(icon);
  }

  ASSERT_FALSE(launcher_->animating_urgent_icons_.empty());
  ASSERT_THAT(launcher_->sources_.GetSource("urgent-timeout"), NotNull());

  icons[0]->SetQuirk(AbstractLauncherIcon::Quirk::URGENT, false);
  launcher_->HandleUrgentIcon(icons[0]);

  ASSERT_EQ(std::set<AbstractLauncherIcon::Ptr>({icons[1]}), launcher_->animating_urgent_icons_);
  EXPECT_THAT(launcher_->sources_.GetSource("urgent-timeout"), NotNull());

  icons[1]->SetQuirk(AbstractLauncherIcon::Quirk::URGENT, false);
  launcher_->HandleUrgentIcon(icons[1]);
  EXPECT_TRUE(launcher_->animating_urgent_icons_.empty());
  EXPECT_THAT(launcher_->sources_.GetSource("urgent-timeout"), IsNull());
}

TEST_F(TestLauncher, UrgentIconTimerTimeout)
{
  auto icons = AddMockIcons(5);
  launcher_->SetHidden(true);

  for (unsigned i = 0; i < icons.size(); ++i)
  {
    bool urgent = ((i % 2) == 0);
    icons[i]->SetQuirk(AbstractLauncherIcon::Quirk::URGENT, urgent);

    InSequence seq;
    EXPECT_CALL(*icons[i], SetQuirk(AbstractLauncherIcon::Quirk::URGENT, false, launcher_->monitor())).Times(urgent ? 1 : 0);
    EXPECT_CALL(*icons[i], SkipQuirkAnimation(AbstractLauncherIcon::Quirk::URGENT, launcher_->monitor())).Times(urgent ? 1 : 0);
    EXPECT_CALL(*icons[i], SetQuirk(AbstractLauncherIcon::Quirk::URGENT, true, launcher_->monitor())).Times(urgent ? 1 : 0);
  }

  ASSERT_EQ(launcher_->urgent_animation_period_, 0);

  // Simulate timer call
  ASSERT_FALSE(launcher_->OnUrgentTimeout());

  EXPECT_THAT(launcher_->urgent_animation_period_, Gt(0));
  EXPECT_THAT(launcher_->sources_.GetSource("urgent-timeout"), NotNull());
}

TEST_F(TestLauncher, UrgentIconsAnimateAfterLauncherIsRevealed)
{
  auto icons = AddMockIcons(5);
  launcher_->SetHidden(true);

  for (unsigned i = 0; i < icons.size(); ++i)
  {
    icons[i]->SetQuirk(AbstractLauncherIcon::Quirk::URGENT, (i % 2) == 0);
    EXPECT_CALL(*icons[i], SetQuirk(AbstractLauncherIcon::Quirk::URGENT, _, _)).Times(0);
    launcher_->HandleUrgentIcon(icons[i]);
  }

  launcher_->SetHidden(false);

  for (auto const& icon : icons)
  {
    Mock::VerifyAndClearExpectations(icon.GetPointer());
    bool urgent = icon->GetQuirk(AbstractLauncherIcon::Quirk::URGENT, launcher_->monitor());

    InSequence seq;
    EXPECT_CALL(*icon, SetQuirk(AbstractLauncherIcon::Quirk::URGENT, false, launcher_->monitor())).Times(urgent ? 1 : 0);
    EXPECT_CALL(*icon, SkipQuirkAnimation(AbstractLauncherIcon::Quirk::URGENT, launcher_->monitor())).Times(urgent ? 1 : 0);
    EXPECT_CALL(*icon, SetQuirk(AbstractLauncherIcon::Quirk::URGENT, true, launcher_->monitor())).Times(urgent ? 1 : 0);

    launcher_->HandleUrgentIcon(icon);
  }

  Utils::WaitPendingEvents();
}

TEST_F(TestLauncher, DesaturateAllIconsOnSpread)
{
  auto const& icons = AddMockIcons(5);
  icons[g_random_int()%icons.size()]->SetQuirk(AbstractLauncherIcon::Quirk::ACTIVE, true);

  WM->SetScaleActive(true);
  WM->initiate_spread.emit();

  Utils::WaitUntilMSec([this, &icons] {
    for (auto const& icon : icons)
    {
      if (!icon->GetQuirk(AbstractLauncherIcon::Quirk::DESAT, launcher_->monitor()))
        return false;
    }
    return true;
  });

  for (auto const& icon : icons)
  {
    for (int i = 0; i < static_cast<int>(monitors::MAX); ++i)
      ASSERT_EQ(launcher_->monitor() == i, icon->GetQuirk(AbstractLauncherIcon::Quirk::DESAT, i));
  }
}

TEST_F(TestLauncher, SaturateAllIconsOnSpreadTerminated)
{
  auto const& icons = AddMockIcons(5);
  icons[g_random_int()%icons.size()]->SetQuirk(AbstractLauncherIcon::Quirk::ACTIVE, true);

  for (auto const& icon : icons)
    icon->SetQuirk(AbstractLauncherIcon::Quirk::DESAT, true);

  WM->terminate_spread.emit();

  for (auto const& icon : icons)
    ASSERT_FALSE(icon->GetQuirk(AbstractLauncherIcon::Quirk::DESAT, launcher_->monitor()));
}

TEST_F(TestLauncher, SaturatesAllIconsOnSpreadWithMouseOver)
{
  auto const& icons = AddMockIcons(5);
  icons[g_random_int()%icons.size()]->SetQuirk(AbstractLauncherIcon::Quirk::ACTIVE, true);

  for (auto const& icon : icons)
    icon->SetQuirk(AbstractLauncherIcon::Quirk::DESAT, true);

  launcher_->SetHover(true);
  WM->SetScaleActive(true);
  WM->initiate_spread.emit();

  Utils::WaitPendingEvents();

  for (auto const& icon : icons)
    ASSERT_FALSE(icon->GetQuirk(AbstractLauncherIcon::Quirk::DESAT, launcher_->monitor()));
}

TEST_F(TestLauncher, DesaturateInactiveIconsOnAppSpread)
{
  auto const& icons = AddMockIcons(5);
  icons[g_random_int()%icons.size()]->SetQuirk(AbstractLauncherIcon::Quirk::ACTIVE, true);

  WM->SetScaleActiveForGroup(true);
  WM->SetScaleActive(true);
  WM->initiate_spread.emit();

  Utils::WaitUntilMSec([this, &icons] {
    for (auto const& icon : icons)
    {
      if (icon->GetQuirk(AbstractLauncherIcon::Quirk::ACTIVE, launcher_->monitor()) == icon->GetQuirk(AbstractLauncherIcon::Quirk::DESAT, launcher_->monitor()))
        return false;
    }
    return true;
  });

  for (auto const& icon : icons)
    ASSERT_NE(icon->GetQuirk(AbstractLauncherIcon::Quirk::ACTIVE, launcher_->monitor()), icon->GetQuirk(AbstractLauncherIcon::Quirk::DESAT, launcher_->monitor()));
}

TEST_F(TestLauncher, SaturatesAllIconsOnAppSpreadMouseMove)
{
  auto const& icons = AddMockIcons(5);
  unsigned active_idx = g_random_int()%icons.size();
  icons[active_idx]->SetQuirk(AbstractLauncherIcon::Quirk::ACTIVE, true);

  launcher_->SetHover(true);
  WM->SetScaleActiveForGroup(true);
  WM->SetScaleActive(true);
  WM->initiate_spread.emit();

  Utils::WaitUntilMSec([this, &icons] {
    for (auto const& icon : icons)
    {
      if (icon->GetQuirk(AbstractLauncherIcon::Quirk::ACTIVE, launcher_->monitor()) == icon->GetQuirk(AbstractLauncherIcon::Quirk::DESAT, launcher_->monitor()))
        return false;
    }
    return true;
  });

  for (auto const& icon : icons)
    ASSERT_NE(icon->GetQuirk(AbstractLauncherIcon::Quirk::ACTIVE, launcher_->monitor()), icon->GetQuirk(AbstractLauncherIcon::Quirk::DESAT, launcher_->monitor()));

  auto const& active_center = icons[active_idx]->GetCenter(launcher_->monitor());
  launcher_->mouse_move.emit(active_center.x, active_center.y, 0, 0, 0, 0);

  for (auto const& icon : icons)
    ASSERT_NE(icon->GetQuirk(AbstractLauncherIcon::Quirk::ACTIVE, launcher_->monitor()), icon->GetQuirk(AbstractLauncherIcon::Quirk::DESAT, launcher_->monitor()));

  auto const& other_center = icons[(active_idx+1)%icons.size()]->GetCenter(launcher_->monitor());
  launcher_->mouse_move.emit(other_center.x, other_center.y, 0, 0, 0, 0);

  for (auto const& icon : icons)
    ASSERT_FALSE(icon->GetQuirk(AbstractLauncherIcon::Quirk::DESAT, launcher_->monitor()));

  launcher_->SetHover(false);
  for (auto const& icon : icons)
    ASSERT_NE(icon->GetQuirk(AbstractLauncherIcon::Quirk::ACTIVE, launcher_->monitor()), icon->GetQuirk(AbstractLauncherIcon::Quirk::DESAT, launcher_->monitor()));

  launcher_->SetHover(true);
  for (auto const& icon : icons)
    ASSERT_FALSE(icon->GetQuirk(AbstractLauncherIcon::Quirk::DESAT, launcher_->monitor()));
}

TEST_F(TestLauncher, DesaturateActiveIconOnAppSpreadIconUpdate)
{
  auto const& icons = AddMockIcons(5);
  unsigned active_idx = g_random_int()%icons.size();
  icons[active_idx]->SetQuirk(AbstractLauncherIcon::Quirk::ACTIVE, true);

  launcher_->SetHover(true);
  WM->SetScaleActiveForGroup(true);
  WM->SetScaleActive(true);
  WM->initiate_spread.emit();

  Utils::WaitPendingEvents();
  for (auto const& icon : icons)
    ASSERT_NE(icon->GetQuirk(AbstractLauncherIcon::Quirk::ACTIVE, launcher_->monitor()), icon->GetQuirk(AbstractLauncherIcon::Quirk::DESAT, launcher_->monitor()));

  unsigned new_active_idx = (active_idx+1)%icons.size();
  icons[active_idx]->SetQuirk(AbstractLauncherIcon::Quirk::ACTIVE, false);
  icons[new_active_idx]->SetQuirk(AbstractLauncherIcon::Quirk::ACTIVE, true);

  WM->terminate_spread.emit();
  WM->initiate_spread.emit();

  Utils::WaitPendingEvents();
  for (auto const& icon : icons)
    ASSERT_NE(icon->GetQuirk(AbstractLauncherIcon::Quirk::ACTIVE, launcher_->monitor()), icon->GetQuirk(AbstractLauncherIcon::Quirk::DESAT, launcher_->monitor()));
}

TEST_F(TestLauncher, HideTooltipOnSpread)
{
  auto icon = AddMockIcons(1).front();
  EXPECT_CALL(*icon, HideTooltip());

  launcher_->SetIconUnderMouse(icon);
  WM->SetScaleActive(true);
  WM->initiate_spread.emit();
}

TEST_F(TestLauncher, HideTooltipOnExpo)
{
  auto icon = AddMockIcons(1).front();
  EXPECT_CALL(*icon, HideTooltip());

  launcher_->SetIconUnderMouse(icon);
  WM->SetExpoActive(true);
  WM->initiate_expo.emit();
}

TEST_F(TestLauncher, IconIsDesaturatedWhenAddedInOverlayMode)
{
  WM->SetScaleActive(true);
  WM->initiate_spread.emit();
  launcher_->SetHover(false);
  ASSERT_TRUE(launcher_->IsOverlayOpen());

  auto icon = AddMockIcons(1).front();

  EXPECT_TRUE(icon->GetQuirk(AbstractLauncherIcon::Quirk::DESAT, launcher_->monitor()));
  EXPECT_FLOAT_EQ(1.0f, icon->GetQuirkProgress(AbstractLauncherIcon::Quirk::DESAT, launcher_->monitor()));
}

TEST_F(TestLauncher, IconIsNotDesaturatedWhenAddedInOverlayModeWithMouseOver)
{
  WM->SetScaleActive(true);
  WM->initiate_spread.emit();
  launcher_->SetHover(true);
  ASSERT_TRUE(launcher_->IsOverlayOpen());

  auto icon = AddMockIcons(1).front();

  EXPECT_FALSE(icon->GetQuirk(AbstractLauncherIcon::Quirk::DESAT, launcher_->monitor()));
  EXPECT_FLOAT_EQ(0.0f, icon->GetQuirkProgress(AbstractLauncherIcon::Quirk::DESAT, launcher_->monitor()));
}

TEST_F(TestLauncher, IconIsNotDesaturatedWhenAddedInNormalMode)
{
  launcher_->SetHover(false);
  ASSERT_FALSE(launcher_->IsOverlayOpen());

  auto icon = AddMockIcons(1).front();

  EXPECT_FALSE(icon->GetQuirk(AbstractLauncherIcon::Quirk::DESAT, launcher_->monitor()));
  EXPECT_FLOAT_EQ(0.0f, icon->GetQuirkProgress(AbstractLauncherIcon::Quirk::DESAT, launcher_->monitor()));
}

} // namespace launcher
} // namespace unity
