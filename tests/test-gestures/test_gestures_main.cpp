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
#include <compiz_mock/core/timer.h>
#include <NuxGraphics/GestureEvent.h>
#include "WindowGestureTargetMock.h"
#include "unityshell_mock.h"

unity::UnityScreenMock concrete_screen_mock;
CompScreenMock *screen_mock = &concrete_screen_mock;
int pointerX_mock = 0;
int pointerY_mock = 0;

CompTimerMock *CompTimerMock::instance = nullptr;

std::map<int, int> g_gesture_event_accept_count;
void nux::GestureEvent::Accept()
{
  g_gesture_event_accept_count[gesture_id_] =
    g_gesture_event_accept_count[gesture_id_] + 1;
}

std::map<int, int> g_gesture_event_reject_count;
void nux::GestureEvent::Reject()
{
  g_gesture_event_reject_count[gesture_id_] =
    g_gesture_event_reject_count[gesture_id_] + 1;
}

Cursor WindowGestureTargetMock::fleur_cursor = 0;
std::set<WindowGestureTargetMock*> g_window_target_mocks;

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);

  int ret = RUN_ALL_TESTS();

  return ret;
}
