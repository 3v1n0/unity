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
 * Authored by: Daniel d'Andrada <daniel.dandrada@canonical.com>
 *
 */

#include <gtest/gtest.h>
#include <compiz_mock/core/core.h>
#include "FakeGestureEvent.h"
#include "unityshell_mock.h"
#include <WindowGestureTarget.h>

class WindowGestureTargetTest : public ::testing::Test
{
 public:
  virtual ~WindowGestureTargetTest() {}

 protected:
  virtual void SetUp()
  {
    screen_mock->width_ = 1280;
    screen_mock->height_ = 1024;
  }

  void PerformPinch(WindowGestureTarget &gesture_target, float peak_radius)
  {
    nux::FakeGestureEvent fake_event;

    fake_event.type = nux::EVENT_GESTURE_BEGIN;
    fake_event.gesture_id = 0;
    fake_event.gesture_classes = nux::PINCH_GESTURE;
    fake_event.is_direct_touch = false;
    // in touch device's coordinate system (because it's not a direct device).
    // Thus not used by WindowCompositor
    fake_event.touches.push_back(nux::TouchPoint(0, 10.0f, 10.0f));
    fake_event.touches.push_back(nux::TouchPoint(1, 20.0f, 20.0f));
    fake_event.touches.push_back(nux::TouchPoint(2, 22.0f, 22.0f));
    fake_event.focus.x = gesture_target.window()->geometry_.centerX();
    fake_event.focus.y = gesture_target.window()->geometry_.centerY();;
    fake_event.radius = 1.0;
    fake_event.is_construction_finished = false;
    gesture_target.GestureEvent(fake_event.ToGestureEvent());

    fake_event.type = nux::EVENT_GESTURE_UPDATE;
    fake_event.radius = peak_radius;
    gesture_target.GestureEvent(fake_event.ToGestureEvent());

    fake_event.type = nux::EVENT_GESTURE_END;
    gesture_target.GestureEvent(fake_event.ToGestureEvent());
  }
};

TEST_F(WindowGestureTargetTest, ThreeFingersDragMovesWindow)
{
  unity::UnityWindowMock window;
  window.geometry_.set(10, 10, 400, 400, 0);
  window.server_geometry_ = window.geometry_;
  window.actions_ = CompWindowActionMoveMask;
  window.state_ = 0;
  window.id_ = 1;

  WindowGestureTarget gesture_target(&window);

  nux::FakeGestureEvent fake_event;

  /* prepare and send the fake event  */
  fake_event.type = nux::EVENT_GESTURE_BEGIN;
  fake_event.gesture_id = 0;
  fake_event.gesture_classes = nux::TOUCH_GESTURE;
  fake_event.is_direct_touch = false;
  fake_event.focus.x = 100.0f; // hits the middle window
  fake_event.focus.y = 100.0f;
  // in touch device's coordinate system (because it's not a direct device).
  // Thus not used by WindowCompositor
  fake_event.touches.push_back(nux::TouchPoint(0, 10.0f, 10.0f));
  fake_event.touches.push_back(nux::TouchPoint(1, 20.0f, 20.0f));
  fake_event.touches.push_back(nux::TouchPoint(2, 22.0f, 22.0f));
  fake_event.is_construction_finished = false;
  fake_event.radius = 1.0f;
  gesture_target.GestureEvent(fake_event.ToGestureEvent());

  ASSERT_FALSE(window.moved_);

  fake_event.type = nux::EVENT_GESTURE_UPDATE;
  fake_event.gesture_classes = nux::TOUCH_GESTURE | nux::DRAG_GESTURE;
  fake_event.delta.x = 10.0f;
  fake_event.delta.y = 20.0f;
  fake_event.focus.x += fake_event.delta.x;
  fake_event.focus.y += fake_event.delta.y;
  fake_event.is_construction_finished = true;
  gesture_target.GestureEvent(fake_event.ToGestureEvent());

  ASSERT_TRUE(window.moved_);
  ASSERT_EQ(fake_event.delta.x, window.movement_x_);
  ASSERT_EQ(fake_event.delta.y, window.movement_y_);
}

TEST_F(WindowGestureTargetTest, ThreeFingersDragDoesntMoveStaticWindow)
{

  unity::UnityWindowMock window;
  window.geometry_.set(10, 10, 400, 400, 0);
  window.server_geometry_ = window.geometry_;
  window.actions_ = 0; /* can't be moved */
  window.state_ = 0;
  window.id_ = 1; 

  WindowGestureTarget gesture_target(&window);

  nux::FakeGestureEvent fake_event;

  /* prepare and send the fake event  */
  fake_event.type = nux::EVENT_GESTURE_BEGIN;
  fake_event.gesture_id = 0;
  fake_event.is_direct_touch = false;
  fake_event.focus.x = 100.0f; // hits the middle window
  fake_event.focus.y = 100.0f;
  // in touch device's coordinate system (because it's not a direct device).
  // Thus not used by WindowCompositor
  fake_event.touches.push_back(nux::TouchPoint(0, 10.0f, 10.0f));
  fake_event.touches.push_back(nux::TouchPoint(1, 20.0f, 20.0f));
  fake_event.touches.push_back(nux::TouchPoint(2, 22.0f, 22.0f));
  fake_event.is_construction_finished = false;
  fake_event.radius = 1.0f;
  gesture_target.GestureEvent(fake_event.ToGestureEvent());

  ASSERT_FALSE(window.moved_);

  fake_event.type = nux::EVENT_GESTURE_UPDATE;
  fake_event.delta.x += 10.0f;
  fake_event.delta.y += 20.0f;
  fake_event.focus.x += fake_event.delta.x;
  fake_event.focus.y += fake_event.delta.y;
  fake_event.is_construction_finished = true;
  gesture_target.GestureEvent(fake_event.ToGestureEvent());

  ASSERT_FALSE(window.moved_);
}

TEST_F(WindowGestureTargetTest, ThreeFingersPinchMaximizesWindow)
{
  unity::UnityWindowMock window;
  window.geometry_.set(10, 10, 400, 400, 0);
  window.server_geometry_ = window.geometry_;
  window.actions_ = CompWindowActionMoveMask;
  window.state_ = 0;
  window.id_ = 1;

  WindowGestureTarget gesture_target(&window);

  PerformPinch(gesture_target, 2.0);

  ASSERT_EQ(1, window.maximize_count_);
  ASSERT_EQ(MAXIMIZE_STATE, window.maximize_state_);
}

TEST_F(WindowGestureTargetTest, ThreeFingersPinchRestoresWindow)
{
  unity::UnityWindowMock window;
  window.geometry_.set(10, 10, 400, 400, 0);
  window.server_geometry_ = window.geometry_;
  window.actions_ = CompWindowActionMoveMask;
  window.state_ = MAXIMIZE_STATE;
  window.id_ = 1;

  WindowGestureTarget gesture_target(&window);

  PerformPinch(gesture_target, 0.3);

  ASSERT_EQ(1, window.maximize_count_);
  ASSERT_EQ(0, window.maximize_state_);
}

TEST_F(WindowGestureTargetTest, MinimalThreeFingersPinchDoesNothing)
{
  unity::UnityWindowMock window;
  window.geometry_.set(10, 10, 400, 400, 0);
  window.server_geometry_ = window.geometry_;
  window.actions_ = CompWindowActionMoveMask;
  window.state_ = 0;
  window.id_ = 1;

  WindowGestureTarget gesture_target(&window);

  PerformPinch(gesture_target, 1.1);

  ASSERT_EQ(0, window.maximize_count_);
}

/* Regression test for lp:979418, where the grab is not removed if the gesture
 * id is 0. */
TEST_F(WindowGestureTargetTest, DragGrabCheck)
{
  screen_mock->grab_count_ = 0;

  unity::UnityWindowMock window;
  window.geometry_.set(10, 10, 400, 400, 0);
  window.server_geometry_ = window.geometry_;
  window.actions_ = CompWindowActionMoveMask;
  window.state_ = 0;
  window.id_ = 1;

  WindowGestureTarget gesture_target(&window);

  /* prepare and send the fake event  */
  nux::FakeGestureEvent fake_event;
  fake_event.type = nux::EVENT_GESTURE_BEGIN;
  fake_event.gesture_id = 0;
  fake_event.is_direct_touch = false;
  fake_event.focus.x = 100.0f; // hits the middle window
  fake_event.focus.y = 100.0f;
  // in touch device's coordinate system (because it's not a direct device).
  // Thus not used by WindowCompositor
  fake_event.touches.push_back(nux::TouchPoint(0, 10.0f, 10.0f));
  fake_event.touches.push_back(nux::TouchPoint(1, 20.0f, 20.0f));
  fake_event.touches.push_back(nux::TouchPoint(2, 22.0f, 22.0f));
  fake_event.is_construction_finished = false;
  gesture_target.GestureEvent(fake_event.ToGestureEvent());

  fake_event.type = nux::EVENT_GESTURE_END;
  gesture_target.GestureEvent(fake_event.ToGestureEvent());

  ASSERT_EQ(0, screen_mock->grab_count_);
}
