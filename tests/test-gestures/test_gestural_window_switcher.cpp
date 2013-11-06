/*
 * This file is part of Unity
 *
 * Copyright (C) 2012 - Canonical Ltd.
 *
 * Unity is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Unity is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Daniel d'Andrada <daniel.dandrada@canonical.com>
 */

#include <gtest/gtest.h>
#include "GesturalWindowSwitcher.h"
#include "FakeGestureEvent.h"
#include "unityshell_mock.h"
#include "compiz_mock/core/timer.h"

using namespace unity;

class GesturalWindowSwitcherTest: public ::testing::Test
{
 public:
  virtual ~GesturalWindowSwitcherTest() {}
  virtual void SetUp()
  {
    unity_screen = unity::UnityScreenMock::get(screen_mock);
    unity_screen->Reset();

    fake_event.gesture_id = 0;
    fake_event.timestamp = 12345; // some arbitrary, big value

    unity_screen->switcher_controller()->view_built.emit();
  }

  void PerformTap()
  {
    fake_event.type = nux::EVENT_GESTURE_BEGIN;
    fake_event.gesture_id += 1;
    fake_event.gesture_classes = nux::TOUCH_GESTURE;
    gestural_switcher.GestureEvent(fake_event.ToGestureEvent());

    fake_event.type = nux::EVENT_GESTURE_UPDATE;
    fake_event.timestamp += 0.2 * CompoundGestureRecognizer::MAX_TAP_TIME;
    gestural_switcher.GestureEvent(fake_event.ToGestureEvent());

    fake_event.type = nux::EVENT_GESTURE_END;
    fake_event.timestamp += 0.6 * CompoundGestureRecognizer::MAX_TAP_TIME;
    gestural_switcher.GestureEvent(fake_event.ToGestureEvent());
  }

  void PerformTapAndHold()
  {
    PerformTap();

    // Hold

    fake_event.type = nux::EVENT_GESTURE_BEGIN;
    fake_event.gesture_id += 1;
    fake_event.timestamp += 0.6 * CompoundGestureRecognizer::MAX_TIME_BETWEEN_GESTURES;
    gestural_switcher.GestureEvent(fake_event.ToGestureEvent());

    fake_event.type = nux::EVENT_GESTURE_UPDATE;
    fake_event.timestamp += 0.2 * CompoundGestureRecognizer::MAX_TAP_TIME;
    gestural_switcher.GestureEvent(fake_event.ToGestureEvent());

    fake_event.type = nux::EVENT_GESTURE_UPDATE;
    fake_event.timestamp += 1.0 * CompoundGestureRecognizer::HOLD_TIME;
    gestural_switcher.GestureEvent(fake_event.ToGestureEvent());
  }

  GesturalWindowSwitcher gestural_switcher;
  nux::FakeGestureEvent fake_event;
  unity::UnityScreenMock *unity_screen;
};

TEST_F(GesturalWindowSwitcherTest, DoubleTapSwitchesWindow)
{
  PerformTap();
  fake_event.timestamp += 0.6 * CompoundGestureRecognizer::MAX_TIME_BETWEEN_GESTURES;
  PerformTap();

  ASSERT_EQ(1, unity_screen->SetUpAndShowSwitcher_count_);
  ASSERT_EQ(0, unity_screen->switcher_controller()->next_count_);
  ASSERT_EQ(0, unity_screen->switcher_controller()->prev_count_);
  ASSERT_TRUE(unity_screen->switcher_controller()->is_visible_);

  // simulate that enough time has passed
  if (CompTimerMock::instance)
    CompTimerMock::instance->ForceTimeout();

  ASSERT_EQ(1, unity_screen->SetUpAndShowSwitcher_count_);
  ASSERT_EQ(0, unity_screen->switcher_controller()->next_count_);
  ASSERT_EQ(0, unity_screen->switcher_controller()->prev_count_);
  ASSERT_FALSE(unity_screen->switcher_controller()->is_visible_);
}

TEST_F(GesturalWindowSwitcherTest, TapAndHoldShowsSwitcher)
{
  PerformTapAndHold();

  // switcher should show up
  ASSERT_EQ(1, unity_screen->SetUpAndShowSwitcher_count_);
  ASSERT_EQ(0, unity_screen->switcher_controller()->next_count_);
  ASSERT_EQ(0, unity_screen->switcher_controller()->prev_count_);
  ASSERT_TRUE(unity_screen->switcher_controller()->is_visible_);

  // simulate that enough idle time has passed
  if (CompTimerMock::instance)
    CompTimerMock::instance->ForceTimeout();

  // nothing should change
  ASSERT_EQ(1, unity_screen->SetUpAndShowSwitcher_count_);
  ASSERT_EQ(0, unity_screen->switcher_controller()->next_count_);
  ASSERT_EQ(0, unity_screen->switcher_controller()->prev_count_);
  ASSERT_TRUE(unity_screen->switcher_controller()->is_visible_);

  // lift fingers. End hold.
  fake_event.type = nux::EVENT_GESTURE_END;
  fake_event.timestamp += 0.1 * CompoundGestureRecognizer::HOLD_TIME;
  gestural_switcher.GestureEvent(fake_event.ToGestureEvent());

  // nothing should change
  ASSERT_EQ(1, unity_screen->SetUpAndShowSwitcher_count_);
  ASSERT_EQ(0, unity_screen->switcher_controller()->next_count_);
  ASSERT_EQ(0, unity_screen->switcher_controller()->prev_count_);
  ASSERT_TRUE(unity_screen->switcher_controller()->is_visible_);

  // simulate that enough idle time has passed
  if (CompTimerMock::instance)
    CompTimerMock::instance->ForceTimeout();

  // switcher should finally be closed
  ASSERT_EQ(1, unity_screen->SetUpAndShowSwitcher_count_);
  ASSERT_EQ(0, unity_screen->switcher_controller()->next_count_);
  ASSERT_EQ(0, unity_screen->switcher_controller()->prev_count_);
  ASSERT_FALSE(unity_screen->switcher_controller()->is_visible_);
}

TEST_F(GesturalWindowSwitcherTest, TapAndHoldAndDragSelectsNextWindow)
{
  PerformTapAndHold();

  if (CompTimerMock::instance)
    CompTimerMock::instance->ForceTimeout();

  ASSERT_EQ(1, unity_screen->SetUpAndShowSwitcher_count_);
  ASSERT_EQ(0, unity_screen->switcher_controller()->next_count_);
  ASSERT_EQ(0, unity_screen->switcher_controller()->prev_count_);
  ASSERT_TRUE(unity_screen->switcher_controller()->is_visible_);

  // Drag far enough to the right.
  fake_event.type = nux::EVENT_GESTURE_UPDATE;
  fake_event.gesture_classes = nux::TOUCH_GESTURE | nux::DRAG_GESTURE;
  fake_event.timestamp += 0.1 * CompoundGestureRecognizer::HOLD_TIME;
  fake_event.delta.x = 0.6 * GesturalWindowSwitcher::DRAG_DELTA_FOR_CHANGING_SELECTION;
  gestural_switcher.GestureEvent(fake_event.ToGestureEvent());

  fake_event.timestamp += 0.1 * CompoundGestureRecognizer::HOLD_TIME;
  fake_event.delta.x = 0.6 * GesturalWindowSwitcher::DRAG_DELTA_FOR_CHANGING_SELECTION;
  gestural_switcher.GestureEvent(fake_event.ToGestureEvent());

  // Selection should have jumped to the next window
  ASSERT_EQ(1, unity_screen->SetUpAndShowSwitcher_count_);
  ASSERT_EQ(1, unity_screen->switcher_controller()->next_count_);
  ASSERT_EQ(0, unity_screen->switcher_controller()->prev_count_);
  ASSERT_TRUE(unity_screen->switcher_controller()->is_visible_);

  // Drag far enough to the left.
  fake_event.timestamp += 0.1 * CompoundGestureRecognizer::HOLD_TIME;
  fake_event.delta.x = -0.6 * GesturalWindowSwitcher::DRAG_DELTA_FOR_CHANGING_SELECTION;
  gestural_switcher.GestureEvent(fake_event.ToGestureEvent());

  fake_event.timestamp += 0.1 * CompoundGestureRecognizer::HOLD_TIME;
  fake_event.delta.x = -0.6 * GesturalWindowSwitcher::DRAG_DELTA_FOR_CHANGING_SELECTION;
  gestural_switcher.GestureEvent(fake_event.ToGestureEvent());

  // Selection should have jumped to the previous window
  ASSERT_EQ(1, unity_screen->SetUpAndShowSwitcher_count_);
  ASSERT_EQ(1, unity_screen->switcher_controller()->next_count_);
  ASSERT_EQ(1, unity_screen->switcher_controller()->prev_count_);
  ASSERT_TRUE(unity_screen->switcher_controller()->is_visible_);

  // End gesture
  fake_event.type = nux::EVENT_GESTURE_END;
  fake_event.timestamp += 0.1 * CompoundGestureRecognizer::HOLD_TIME;
  gestural_switcher.GestureEvent(fake_event.ToGestureEvent());

  // Switcher should have been closed at once
  ASSERT_EQ(1, unity_screen->SetUpAndShowSwitcher_count_);
  ASSERT_EQ(1, unity_screen->switcher_controller()->next_count_);
  ASSERT_EQ(1, unity_screen->switcher_controller()->prev_count_);
  ASSERT_FALSE(unity_screen->switcher_controller()->is_visible_);
}

TEST_F(GesturalWindowSwitcherTest, NewDragAfterTapAndHoldSelectsNextWindow)
{
  PerformTapAndHold();

  if (CompTimerMock::instance)
    CompTimerMock::instance->ForceTimeout();

  ASSERT_EQ(1, unity_screen->SetUpAndShowSwitcher_count_);
  ASSERT_EQ(0, unity_screen->switcher_controller()->next_count_);
  ASSERT_EQ(0, unity_screen->switcher_controller()->prev_count_);
  ASSERT_TRUE(unity_screen->switcher_controller()->is_visible_);

  // End hold gesture
  fake_event.type = nux::EVENT_GESTURE_END;
  fake_event.timestamp += 0.1 * CompoundGestureRecognizer::HOLD_TIME;
  gestural_switcher.GestureEvent(fake_event.ToGestureEvent());

  // Start a new gesture and drag far enough to the right.

  fake_event.type = nux::EVENT_GESTURE_BEGIN;
  fake_event.gesture_id += 1;
  fake_event.gesture_classes = nux::TOUCH_GESTURE;
  fake_event.timestamp += 0.6 * CompoundGestureRecognizer::HOLD_TIME;
  gestural_switcher.GestureEvent(fake_event.ToGestureEvent());

  fake_event.type = nux::EVENT_GESTURE_UPDATE;
  fake_event.gesture_classes |= nux::DRAG_GESTURE;
  fake_event.timestamp += 0.1 * CompoundGestureRecognizer::HOLD_TIME;
  fake_event.delta.x = 0.6 * GesturalWindowSwitcher::DRAG_DELTA_FOR_CHANGING_SELECTION;
  gestural_switcher.GestureEvent(fake_event.ToGestureEvent());

  fake_event.timestamp += 0.1 * CompoundGestureRecognizer::HOLD_TIME;
  fake_event.delta.x = 0.6 * GesturalWindowSwitcher::DRAG_DELTA_FOR_CHANGING_SELECTION;
  gestural_switcher.GestureEvent(fake_event.ToGestureEvent());

  ASSERT_EQ(1, unity_screen->SetUpAndShowSwitcher_count_);
  ASSERT_EQ(1, unity_screen->switcher_controller()->next_count_);
  ASSERT_EQ(0, unity_screen->switcher_controller()->prev_count_);
  ASSERT_TRUE(unity_screen->switcher_controller()->is_visible_);

  fake_event.type = nux::EVENT_GESTURE_END;
  fake_event.timestamp += 0.1 * CompoundGestureRecognizer::HOLD_TIME;
  gestural_switcher.GestureEvent(fake_event.ToGestureEvent());

  // switcher should have been closed
  ASSERT_EQ(1, unity_screen->SetUpAndShowSwitcher_count_);
  ASSERT_EQ(1, unity_screen->switcher_controller()->next_count_);
  ASSERT_EQ(0, unity_screen->switcher_controller()->prev_count_);
  ASSERT_FALSE(unity_screen->switcher_controller()->is_visible_);
}

TEST_F(GesturalWindowSwitcherTest, ClickAfterTapAndHoldSelectsWindow)
{
  PerformTapAndHold();

  if (CompTimerMock::instance)
    CompTimerMock::instance->ForceTimeout();

  ASSERT_EQ(1, unity_screen->SetUpAndShowSwitcher_count_);
  ASSERT_EQ(0, unity_screen->switcher_controller()->next_count_);
  ASSERT_EQ(0, unity_screen->switcher_controller()->prev_count_);
  ASSERT_TRUE(unity_screen->switcher_controller()->is_visible_);

  // lift fingers. End hold.
  fake_event.type = nux::EVENT_GESTURE_END;
  fake_event.timestamp += 0.1 * CompoundGestureRecognizer::HOLD_TIME;
  gestural_switcher.GestureEvent(fake_event.ToGestureEvent());

  // click on position (12, 23)
  unity_screen->switcher_controller()->view_.mouse_down.emit(12, 23, 0, 0);
  unity_screen->switcher_controller()->view_.mouse_up.emit(12, 23, 0, 0);

  // Should have selected the icon index corresponding to the
  // position clicked.
  ASSERT_EQ(1, unity_screen->SetUpAndShowSwitcher_count_);
  ASSERT_EQ(0, unity_screen->switcher_controller()->next_count_);
  ASSERT_EQ(0, unity_screen->switcher_controller()->prev_count_);
  ASSERT_TRUE(unity_screen->switcher_controller()->is_visible_);
  ASSERT_EQ(12*23, unity_screen->switcher_controller()->index_selected_);

  // simulate that enough time has passed
  if (CompTimerMock::instance)
    CompTimerMock::instance->ForceTimeout();

  ASSERT_FALSE(unity_screen->switcher_controller()->is_visible_);
}

TEST_F(GesturalWindowSwitcherTest, MouseDragAfterTapAndHoldSelectsNextWindow)
{
  unity::switcher::SwitcherViewMock &switcher_view =
    unity_screen->switcher_controller()->view_;
  int drag_delta = GesturalWindowSwitcher::DRAG_DELTA_FOR_CHANGING_SELECTION * 1.5f;

  PerformTapAndHold();

  if (CompTimerMock::instance)
    CompTimerMock::instance->ForceTimeout();

  ASSERT_EQ(1, unity_screen->SetUpAndShowSwitcher_count_);
  ASSERT_EQ(0, unity_screen->switcher_controller()->next_count_);
  ASSERT_EQ(0, unity_screen->switcher_controller()->prev_count_);
  ASSERT_TRUE(unity_screen->switcher_controller()->is_visible_);

  // lift fingers. End hold.
  fake_event.type = nux::EVENT_GESTURE_END;
  fake_event.timestamp += 0.1 * CompoundGestureRecognizer::HOLD_TIME;
  gestural_switcher.GestureEvent(fake_event.ToGestureEvent());

  // drag right far enough to trigger the selection of the next item
  switcher_view.mouse_down.emit(12, 23, 0, 0);
  switcher_view.mouse_drag.emit(12 + drag_delta, 23, drag_delta, 0, 0, 0);
  switcher_view.mouse_up.emit(12 + drag_delta, 23, 0, 0);

  // Should selected the next icon and close the switcher right away
  ASSERT_EQ(1, unity_screen->SetUpAndShowSwitcher_count_);
  ASSERT_EQ(1, unity_screen->switcher_controller()->next_count_);
  ASSERT_EQ(0, unity_screen->switcher_controller()->prev_count_);
  ASSERT_FALSE(unity_screen->switcher_controller()->is_visible_);
  ASSERT_EQ(-1, unity_screen->switcher_controller()->index_selected_);
}
