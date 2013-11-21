/*
 * UnityGestureTarget.h
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

#ifndef UNITY_GESTURE_TARGET_H
#define UNITY_GESTURE_TARGET_H

#include <Nux/Gesture.h>

/*
  Target for Unity-level gestures.
  I.e., for gestures that act on Unity elements, such as
  the dash or launcher.
 */
class UnityGestureTarget : public nux::GestureTarget
{
  public:
    UnityGestureTarget();
    virtual ~UnityGestureTarget() {}

    virtual nux::GestureDeliveryRequest GestureEvent(const nux::GestureEvent &event);

  private:
    nux::ObjectWeakPtr<nux::InputArea> launcher;
};

#endif // UNITY_GESTURE_TARGET_H
