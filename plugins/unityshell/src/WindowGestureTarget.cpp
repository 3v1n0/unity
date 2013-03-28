/*
 * WindowGestureTarget.cpp
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

#include "WindowGestureTarget.h"

#include <Nux/Nux.h> // otherwise unityshell.h inclusion will cause failures
#include "unityshell.h"

// To make the gesture tests pass, this has to be a local include.
#include "PluginAdapter.h"

using namespace nux;

Cursor WindowGestureTarget::fleur_cursor = 0;

WindowGestureTarget::WindowGestureTarget(CompWindow *window)
  : window_(window), drag_grab_(0), started_window_move_(false),
    window_restored_by_pinch_(false)
{
  // A workaround for the lack of weak pointers.
  unity::UnityWindow *unity_window = unity::UnityWindow::get(window);

  connection_window_destruction =
    unity_window->being_destroyed.connect(
        sigc::mem_fun(this, &WindowGestureTarget::NullifyWindowPointer));
}

WindowGestureTarget::~WindowGestureTarget()
{
  connection_window_destruction.disconnect();
  if (drag_grab_)
  {
    if (window_)
      window_->ungrabNotify();
    screen->removeGrab(drag_grab_, NULL);
  }
}

void WindowGestureTarget::NullifyWindowPointer()
{
  window_ = nullptr;
}

GestureDeliveryRequest WindowGestureTarget::GestureEvent(const nux::GestureEvent &event)
{
  if (!window_)
    return GestureDeliveryRequest::NONE;

  switch (event.type)
  {
    case nux::EVENT_GESTURE_BEGIN:
      PluginAdapter::Default().ShowGrabHandles(window_, false);
      break;
    case EVENT_GESTURE_UPDATE:
      if (event.GetGestureClasses() & PINCH_GESTURE)
        MaximizeOrRestoreWindowDueToPinch(event);
      if (event.GetGestureClasses() & DRAG_GESTURE)
      {
        if (WindowCanMove())
        {
          if (!started_window_move_)
          {
            StartWindowMove(event);
            started_window_move_ = true;
          }
          MoveWindow(event);
        }
      }
      break;
    default: // EVENT_GESTURE_END | EVENT_GESTURE_LOST
      if (event.GetGestureClasses() & DRAG_GESTURE)
      {
        EndWindowMove(event);
        started_window_move_ = false;
      }
      PluginAdapter::Default().ShowGrabHandles(window_, true);
      break;
  };

  return GestureDeliveryRequest::NONE;
}

bool WindowGestureTarget::WindowCanMove()
{
  if (!(window_->actions() & CompWindowActionMoveMask))
    return false;

  /* Don't allow windows to be dragged if completely maximized */
  if ((window_->state() & MAXIMIZE_STATE) == MAXIMIZE_STATE)
    return false;

  /* Don't start moving a window that has just been restored. The user is likely
     still performing the pinch and not expecting the window to start moving */
  if (window_restored_by_pinch_)
    return false;

  return true;
}

void WindowGestureTarget::MaximizeOrRestoreWindowDueToPinch(const nux::GestureEvent &event)
{
  if (event.GetRadius() > 1.25f)
  {
    window_->maximize(MAXIMIZE_STATE);
    RemoveDragGrab();
    window_restored_by_pinch_ = false;
  }
  else if (event.GetRadius() < 0.8f)
  {
    if (window_->state() & MAXIMIZE_STATE)
    {
      window_->maximize(0);
      RemoveDragGrab();
      window_restored_by_pinch_ = true;
    }
  }
}

void WindowGestureTarget::StartWindowMove(const nux::GestureEvent &event)
{
  if (!event.IsDirectTouch())
  {
    drag_grab_ = screen->pushGrab(fleur_cursor, "unity");
    window_->grabNotify(window_->serverGeometry().x(),
                        window_->serverGeometry().y(),
                        0,
                        CompWindowGrabMoveMask | CompWindowGrabButtonMask);
  }
}

void WindowGestureTarget::MoveWindow(const nux::GestureEvent &event)
{
  const nux::Point2D<float> &delta = event.GetDelta();

  unsigned int px = std::max(std::min(pointerX + static_cast<int>(delta.x),
                                      screen->width()),
                             0);

  unsigned int py = std::max(std::min(pointerY + static_cast<int>(delta.y),
                                      screen->height()),
                             0);

  if (window_->state() & CompWindowStateMaximizedVertMask)
    py = pointerY;
  if (window_->state() & CompWindowStateMaximizedHorzMask)
    px = pointerX;

  if (!event.IsDirectTouch())
  {
    /* FIXME: CompScreen::warpPointer filters out motion events which
       other plugins may need to process, but for most cases in core
       they should be filtered out. */
    XWarpPointer(screen->dpy (),
        None, screen->root (),
        0, 0, 0, 0,
        px, py);
  }

  XSync(screen->dpy (), false);
  window_->move(px - pointerX, py - pointerY, false);

  pointerX = px;
  pointerY = py;
}

void WindowGestureTarget::EndWindowMove(const nux::GestureEvent &event)
{
  window_->ungrabNotify();
  RemoveDragGrab();
}

void WindowGestureTarget::RemoveDragGrab()
{
  if (drag_grab_)
  {
    screen->removeGrab(drag_grab_, NULL);
    drag_grab_ = 0;
  }
}

bool WindowGestureTarget::Equals(const nux::GestureTarget& other) const
{
  const WindowGestureTarget *window_target = dynamic_cast<const WindowGestureTarget *>(&other);

  if (window_target)
  {
    if (window_ && window_target->window_)
      return window_->id() == window_target->window_->id();
    else
      return window_ == window_target->window_;
  }
  else
  {
    return false;
  }
}
