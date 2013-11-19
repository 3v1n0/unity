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
#include "UnityGestureBroker.h"
#include "FakeGestureEvent.h"
#include "unityshell_mock.h"
#include "WindowGestureTargetMock.h"

class GestureBrokerTest : public ::testing::Test
{
 public:
  virtual ~GestureBrokerTest() {}
 protected:
  virtual void SetUp()
  {
    screen_mock->width_ = 1280;
    screen_mock->height_ = 1024;

    GenerateWindows();
  }

 private:
  void GenerateWindows()
  {
    /* remove windows from previous test */
    for (auto window : screen_mock->client_list_stacking_) {
      delete window;
    }
    screen_mock->client_list_stacking_.clear();

    /* and generate new ones */
    CompWindowMock *window;

    /* the root window */
    window = new unity::UnityWindowMock;
    window->id_ = 0;
    /* x, y, width, height, border */
    window->geometry_.set(0, 0, screen_mock->width(), screen_mock->height(), 0);
    window->server_geometry_ = window->geometry_;
    window->actions_ = 0;
    window->state_ = 0;
    screen_mock->client_list_stacking_.push_back(window);

    /* middle window */
    window = new unity::UnityWindowMock;
    window->id_ = 1;
    window->geometry_.set(10, 10, 400, 400, 0);
    window->server_geometry_ = window->geometry_;
    window->actions_ = CompWindowActionMoveMask;
    window->state_ = 0;
    screen_mock->client_list_stacking_.push_back(window);

    /* top-level window */
    window = new unity::UnityWindowMock;
    window->id_ = 2;
    window->geometry_.set(500, 500, 410, 410, 0);
    window->server_geometry_ = window->geometry_;
    window->actions_ = CompWindowActionMoveMask;
    window->state_ = 0;
    screen_mock->client_list_stacking_.push_back(window);

    screen_mock->client_list_ = screen_mock->client_list_stacking_;
    std::reverse(screen_mock->client_list_.begin(),
                 screen_mock->client_list_.end());
  }
};

/*
 Tests that events from a three-fingers' Touch gesture goes to the
 correct window. I.e., to the window that lies where the gesture starts.
 */
TEST_F(GestureBrokerTest, ThreeFingersTouchHitsCorrectWindow)
{
  UnityGestureBroker gesture_broker;
  CompWindowMock *middle_window = screen_mock->client_list_stacking_[1];
  nux::FakeGestureEvent fake_event;

  // init counters
  g_gesture_event_accept_count[0] = 0;
  g_gesture_event_reject_count[0] = 0;

  /* prepare and send the fake event  */
  fake_event.type = nux::EVENT_GESTURE_BEGIN;
  fake_event.gesture_id = 0;
  fake_event.is_direct_touch = true;
  fake_event.touches.push_back(nux::TouchPoint(0, 100.0f, 100.0f)); // hits the middle window
  fake_event.touches.push_back(nux::TouchPoint(1, 120.0f, 120.0f));
  fake_event.touches.push_back(nux::TouchPoint(2, 122.0f, 122.0f));
  fake_event.is_construction_finished = false;
  gesture_broker.ProcessGestureBegin(fake_event.ToGestureEvent());

  // Gesture shouldn't be accepted as constructions hasn't finished
  ASSERT_EQ(0, g_gesture_event_accept_count[0]);
  ASSERT_EQ(0, g_gesture_event_reject_count[0]);
  ASSERT_EQ(1, g_window_target_mocks.size());
  WindowGestureTargetMock *target_mock = *g_window_target_mocks.begin();
  ASSERT_TRUE(target_mock->window == middle_window);
  // No events yet as the broker didn't accept the gesture yet
  ASSERT_EQ(0, target_mock->events_received.size());

  fake_event.type = nux::EVENT_GESTURE_UPDATE;
  fake_event.touches.push_back(nux::TouchPoint(4, 132.0f, 142.0f));
  fake_event.is_construction_finished = true;
  gesture_broker.ProcessGestureUpdate(fake_event.ToGestureEvent());

  // Gesture should have been accepted now since the construction has finished.
  ASSERT_EQ(1, g_gesture_event_accept_count[0]);
  ASSERT_EQ(0, g_gesture_event_reject_count[0]);
  // Check that this gesture target is still valid
  ASSERT_EQ(1, g_window_target_mocks.count(target_mock));
  // Gesture events should have been sent to the target by now
  ASSERT_EQ(2, target_mock->events_received.size());
}
