/*
 * UnityGestureBroker.cpp
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

#include "UnityGestureBroker.h"
#include "UnityGestureTarget.h"
#include "WindowGestureTarget.h"

#include <X11/cursorfont.h>

UnityGestureBroker::UnityGestureBroker()
  : nux::GestureBroker()
{
  g_assert(WindowGestureTarget::fleur_cursor == 0);
  WindowGestureTarget::fleur_cursor = XCreateFontCursor (screen->dpy (), XC_fleur);

  unity_target.reset(new UnityGestureTarget);
  gestural_window_switcher_.reset(new unity::GesturalWindowSwitcher);
}

UnityGestureBroker::~UnityGestureBroker()
{
  if (WindowGestureTarget::fleur_cursor)
  {
    XFreeCursor (screen->dpy (), WindowGestureTarget::fleur_cursor);
    WindowGestureTarget::fleur_cursor = 0;
  }
}

std::vector<nux::ShPtGestureTarget>
UnityGestureBroker::FindGestureTargets(const nux::GestureEvent &event)
{
  std::vector<nux::ShPtGestureTarget> targets;

  const std::vector<nux::TouchPoint> &touches = event.GetTouches();

  if (touches.size() == 4)
  {
    targets.push_back(unity_target);
  }
  else if (touches.size() == 3)
  {
    targets.push_back(gestural_window_switcher_);

    CompWindow *window = FindWindowHitByGesture(event);
    if (window && event.IsDirectTouch())
    {
      targets.push_back(nux::ShPtGestureTarget(new WindowGestureTarget(window)));
    }
  }

  return targets;
}

CompWindow *UnityGestureBroker::FindWindowHitByGesture(const nux::GestureEvent &event)
{
  if (event.IsDirectTouch())
  {
    /* If a direct device is being used (e.g., a touchscreen), all touch
       points must hit the same window */
    CompWindow *last_window = nullptr;
    const std::vector<nux::TouchPoint> &touches = event.GetTouches();
    for (auto touch : touches)
    {
      CompWindow *window = FindCompWindowAtPos(touch.x, touch.y);
      if (last_window)
      {
        if (window != last_window)
        {
          return nullptr;
        }
      }
      else
      {
        last_window = window;
      }
    }

    return last_window;
  }
  else
  {
    /* If a indirect device is being used (e.g., a trackpad), the individual
       touch points are not in screen coordinates and therefore it doesn't make
       sense to hit-test them individually against the window tree. Instead,
       we use just the focus point, which is the same as the cursor
       position in this case (which is in screen coordinates). */
    return FindCompWindowAtPos(event.GetFocus().x, event.GetFocus().y);
  }
}

CompWindow* UnityGestureBroker::FindCompWindowAtPos(int pos_x, int pos_y)
{
  const CompWindowVector& client_list_stacking = screen->clientList(true);

  for (auto iter = client_list_stacking.rbegin(),
       end = client_list_stacking.rend();
       iter != end; ++iter)
  {
    CompWindow* window = *iter;

    if (window->minimized())
      continue;

    if (window->state() & CompWindowStateHiddenMask)
      continue;

    if (pos_x >= window->x() && pos_x <= (window->width() + window->x())
        &&
        pos_y >= window->y() && pos_y <= (window->height() + window->y()))
      return window;
  }

  return nullptr;
}
