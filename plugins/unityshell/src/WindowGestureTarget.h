/*
 * WindowGestureTarget.h
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
 * along with Unity; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#ifndef WINDOW_GESTURE_TARGET_H
#define WINDOW_GESTURE_TARGET_H

#include <Nux/Gesture.h>
#include <UnityCore/ConnectionManager.h>

#include <core/core.h> // compiz stuff

class WindowGestureTarget : public nux::GestureTarget
{
  public:
    WindowGestureTarget(CompWindow *window);
    virtual ~WindowGestureTarget();

    virtual nux::GestureDeliveryRequest GestureEvent(const nux::GestureEvent &event);

    static Cursor fleur_cursor;

    CompWindow *window() {return window_;}
  private:
    virtual bool Equals(const nux::GestureTarget& other) const;
    void StartWindowMove(const nux::GestureEvent &event);
    void MoveWindow(const nux::GestureEvent &event);
    void EndWindowMove(const nux::GestureEvent &event);
    void MaximizeOrRestoreWindowDueToPinch(const nux::GestureEvent &event);
    bool WindowCanMove();
    void NullifyWindowPointer();
    void RemoveDragGrab();
    CompWindow *window_;
    CompScreen::GrabHandle drag_grab_;
    bool started_window_move_;
    bool window_restored_by_pinch_;
    unity::connection::Wrapper window_destruction_conn_;
};

#endif // WINDOW_GESTURE_TARGET_H
