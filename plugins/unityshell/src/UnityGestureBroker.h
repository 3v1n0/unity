/*
 * UnityGestureBroker.h
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

#ifndef UNITY_GESTURE_BROKER
#define UNITY_GESTURE_BROKER

#include <core/core.h>

#include <Nux/GestureBroker.h>
#include "GesturalWindowSwitcher.h"

class UnityGestureBroker : public nux::GestureBroker
{
public:
  UnityGestureBroker();
  virtual ~UnityGestureBroker();

private:
  std::vector<nux::ShPtGestureTarget>
    virtual FindGestureTargets(const nux::GestureEvent &event);

  CompWindow *FindWindowHitByGesture(const nux::GestureEvent &event);

  /*!
    Returns the top-most CompWindow at the given position, if any.
   */
  CompWindow* FindCompWindowAtPos(int pos_x, int pos_y);

  nux::ShPtGestureTarget unity_target;
  unity::ShPtGesturalWindowSwitcher gestural_window_switcher_;
};

#endif // UNITY_GESTURE_BROKER
