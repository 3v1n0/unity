/*
 * UnityGestureTarget.cpp
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

#include "UnityGestureTarget.h"

#include <Nux/Nux.h> // otherwise unityshell.h inclusion will cause failures
#include "unityshell.h"
#include "Launcher.h"

#include "UBusMessages.h"
#include "UBusWrapper.h"

using namespace nux;

UnityGestureTarget::UnityGestureTarget()
{
  launcher = &unity::UnityScreen::get(screen)->launcher_controller()->launcher();
}

GestureDeliveryRequest UnityGestureTarget::GestureEvent(const nux::GestureEvent &event)
{
  if (event.GetGestureClasses() & DRAG_GESTURE)
  {
    if (launcher.IsValid())
      launcher->GestureEvent(event);
  }
  else if (event.GetGestureClasses() == TAP_GESTURE
      && event.type == EVENT_GESTURE_END)
  {
    UBusManager::SendMessage(UBUS_DASH_EXTERNAL_ACTIVATION);
  }

  return GestureDeliveryRequest::NONE;
}
