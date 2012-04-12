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
#include "GestureEngine.h"

CompScreenMock concrete_screen_mock;
CompScreenMock *screen_mock = &concrete_screen_mock;
int pointerX_mock = 0;
int pointerY_mock = 0;

class GestureEngineTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    screen_mock->_width = 1280;
    screen_mock->_height = 1024;

    GenerateWindows();
  }

  void PerformPinch(GestureEngine &gesture_engine, float peak_radius) {
    CompWindowMock *middle_window = screen_mock->_client_list_stacking[1];

    GeisAdapterMock::GeisTouchData touch_data;
    touch_data.id = 1;
    touch_data.touches = 3;
    touch_data.window = 123;
    touch_data.focus_x = 100; /* hits the middle window */
    touch_data.focus_y = 100;
    gesture_engine.OnTouchStart(&touch_data);

    GeisAdapterMock::GeisPinchData pinch_data;
    pinch_data.id = 1;
    pinch_data.touches = 3;
    pinch_data.window = 123;
    pinch_data.focus_x = 100; /* hits the middle window */
    pinch_data.focus_y = 100;
    pinch_data.radius = 1.0;
    gesture_engine.OnPinchStart(&pinch_data);

    touch_data.focus_x += 10;
    touch_data.focus_y += 20;
    gesture_engine.OnTouchUpdate(&touch_data);

    pinch_data.focus_x += 10;
    pinch_data.focus_y += 20;
    pinch_data.radius = peak_radius;
    gesture_engine.OnPinchUpdate(&pinch_data);

    gesture_engine.OnTouchFinish(&touch_data);
    gesture_engine.OnPinchFinish(&pinch_data);
  }

 private:
  void GenerateWindows() {
    /* remove windows from previous test */
    for (auto window : screen_mock->_client_list_stacking) {
      delete window;
    }
    screen_mock->_client_list_stacking.clear();

    /* and generate new ones */
    CompWindowMock *window;

    /* the root window */
    window = new CompWindowMock;
    /* x, y, width, height, border */
    window->_geometry.set(0, 0, screen_mock->width(), screen_mock->height(), 0);
    window->_serverGeometry = window->_geometry;
    window->_actions = 0;
    window->_state = 0;
    screen_mock->_client_list_stacking.push_back(window);

    /* middle window */
    window = new CompWindowMock;
    window->_geometry.set(10, 10, 400, 400, 0);
    window->_serverGeometry = window->_geometry;
    window->_actions = CompWindowActionMoveMask;
    window->_state = 0;
    screen_mock->_client_list_stacking.push_back(window);

    /* top-level window */
    window = new CompWindowMock;
    window->_geometry.set(500, 500, 410, 410, 0);
    window->_serverGeometry = window->_geometry;
    window->_actions = CompWindowActionMoveMask;
    window->_state = 0;
    screen_mock->_client_list_stacking.push_back(window);

    screen_mock->_client_list = screen_mock->_client_list_stacking;
    std::reverse(screen_mock->_client_list.begin(),
                 screen_mock->_client_list.end());
  }
};

TEST_F(GestureEngineTest, ThreeFingersDragMovesWindow)
{
  GestureEngine gestureEngine(screen_mock);
  CompWindowMock *middle_window = screen_mock->_client_list_stacking[1];

  GeisAdapterMock::GeisTouchData touch_data;
  touch_data.id = 1;
  touch_data.touches = 3;
  touch_data.window = 123;
  touch_data.focus_x = 100; /* hits the middle window */
  touch_data.focus_y = 100;
  gestureEngine.OnTouchStart(&touch_data);

  GeisAdapterMock::GeisDragData drag_data;
  drag_data.id = 1;
  drag_data.touches = 3;
  drag_data.window = 123;
  drag_data.focus_x = 100; /* hits the middle window */
  drag_data.focus_y = 100;
  gestureEngine.OnDragStart(&drag_data);

  ASSERT_FALSE(middle_window->_moved);

  touch_data.focus_x += 10;
  touch_data.focus_y += 20;
  gestureEngine.OnTouchUpdate(&touch_data);

  drag_data.delta_x = 10;
  drag_data.delta_y = 20;
  drag_data.focus_x += drag_data.delta_x;
  drag_data.focus_y += drag_data.delta_x;
  gestureEngine.OnDragUpdate(&drag_data);

  ASSERT_TRUE(middle_window->_moved);
  ASSERT_EQ(drag_data.delta_x, middle_window->_movement_x);
  ASSERT_EQ(drag_data.delta_y, middle_window->_movement_y);
}

TEST_F(GestureEngineTest, ThreeFingersDragDoesntMoveStaticWindow)
{
  GestureEngine gestureEngine(screen_mock);
  CompWindowMock *middle_window = screen_mock->_client_list_stacking[1];

  /* can't be moved */
  middle_window->_actions = 0;

  GeisAdapterMock::GeisTouchData touch_data;
  touch_data.id = 1;
  touch_data.touches = 3;
  touch_data.window = 123;
  touch_data.focus_x = 100; /* hits the middle window */
  touch_data.focus_y = 100;
  gestureEngine.OnTouchStart(&touch_data);

  GeisAdapterMock::GeisDragData drag_data;
  drag_data.id = 1;
  drag_data.touches = 3;
  drag_data.window = 123;
  drag_data.focus_x = 100; /* hits the middle window */
  drag_data.focus_y = 100;
  gestureEngine.OnDragStart(&drag_data);

  ASSERT_FALSE(middle_window->_moved);

  touch_data.focus_x += 10;
  touch_data.focus_y += 20;
  gestureEngine.OnTouchUpdate(&touch_data);

  drag_data.delta_x = 10;
  drag_data.delta_y = 20;
  drag_data.focus_x += drag_data.delta_x;
  drag_data.focus_y += drag_data.delta_x;
  gestureEngine.OnDragUpdate(&drag_data);

  ASSERT_FALSE(middle_window->_moved);
}

TEST_F(GestureEngineTest, ThreeFingersPinchMaximizesWindow)
{
  GestureEngine gesture_engine(screen_mock);
  CompWindowMock *middle_window = screen_mock->_client_list_stacking[1];

  PerformPinch(gesture_engine, 2.0);

  ASSERT_EQ(1, middle_window->_maximize_count);
  ASSERT_EQ(MAXIMIZE_STATE, middle_window->_maximize_state);
}

TEST_F(GestureEngineTest, ThreeFingersPinchRestoresWindow)
{
  GestureEngine gesture_engine(screen_mock);
  CompWindowMock *middle_window = screen_mock->_client_list_stacking[1];

  PerformPinch(gesture_engine, 0.3);

  ASSERT_EQ(1, middle_window->_maximize_count);
  ASSERT_EQ(0, middle_window->_maximize_state);
}

TEST_F(GestureEngineTest, MinimalThreeFingersPinchDoesNothing)
{
  GestureEngine gesture_engine(screen_mock);
  CompWindowMock *middle_window = screen_mock->_client_list_stacking[1];

  PerformPinch(gesture_engine, 1.1);

  ASSERT_EQ(0, middle_window->_maximize_count);
}

/* Regression test for lp:979418, where the grab is not removed if the gesture
 * id is 0. */
TEST_F(GestureEngineTest, DragGrabCheck)
{
  screen_mock->_grab_count = 0;

  GestureEngine gesture_engine(screen_mock);

  GeisAdapterMock::GeisDragData drag_data;
  drag_data.id = 0;
  drag_data.touches = 3;
  drag_data.window = 123;
  drag_data.focus_x = 100; /* hits the middle window */
  drag_data.focus_y = 100;
  gesture_engine.OnDragStart(&drag_data);

  gesture_engine.OnDragFinish(&drag_data);

  ASSERT_EQ(0, screen_mock->_grab_count);
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);

  int ret = RUN_ALL_TESTS();

  return ret;
}
