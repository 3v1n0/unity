/*
 * GestureEngine.cpp
 * This file is part of Unity
 *
 * Copyright (C) 2011 - Canonical Ltd.
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

#include <X11/cursorfont.h>

#include "ubus-server.h"
#include "UBusMessages.h"
#include "GestureEngine.h"
#include "PluginAdapter.h"

GestureEngine::GestureEngine(CompScreen* screen)
{
  _screen = screen;

  _drag_id = 0;
  _pinch_id = 0;
  _touch_id = 0;
  _drag_grab = 0;
  _pinch_grab = 0;

  GeisAdapter* adapter = GeisAdapter::Default(screen);

  adapter->tap.connect(sigc::mem_fun(this, &GestureEngine::OnTap));

  adapter->drag_start.connect(sigc::mem_fun(this, &GestureEngine::OnDragStart));
  adapter->drag_update.connect(sigc::mem_fun(this, &GestureEngine::OnDragUpdate));
  adapter->drag_finish.connect(sigc::mem_fun(this, &GestureEngine::OnDragFinish));

  adapter->rotate_start.connect(sigc::mem_fun(this, &GestureEngine::OnRotateStart));
  adapter->rotate_update.connect(sigc::mem_fun(this, &GestureEngine::OnRotateUpdate));
  adapter->rotate_finish.connect(sigc::mem_fun(this, &GestureEngine::OnRotateFinish));

  adapter->pinch_start.connect(sigc::mem_fun(this, &GestureEngine::OnPinchStart));
  adapter->pinch_update.connect(sigc::mem_fun(this, &GestureEngine::OnPinchUpdate));
  adapter->pinch_finish.connect(sigc::mem_fun(this, &GestureEngine::OnPinchFinish));

  adapter->touch_start.connect(sigc::mem_fun(this, &GestureEngine::OnTouchStart));
  adapter->touch_update.connect(sigc::mem_fun(this, &GestureEngine::OnTouchUpdate));
  adapter->touch_finish.connect(sigc::mem_fun(this, &GestureEngine::OnTouchFinish));
}

GestureEngine::~GestureEngine()
{

}

void
GestureEngine::OnTap(GeisAdapter::GeisTapData* data)
{
  if (data->touches == 4)
  {
    UBusServer* ubus = ubus_server_get_default();
    ubus_server_send_message(ubus, UBUS_DASH_EXTERNAL_ACTIVATION, NULL);
  }
}

CompWindow*
GestureEngine::FindCompWindow(Window window)
{
  CompWindow* result = _screen->findTopLevelWindow(window);

  while (!result)
  {
    Window parent, root;
    Window* children = NULL;
    unsigned int nchildren;

    XQueryTree(_screen->dpy(), window, &root, &parent, &children, &nchildren);

    if (children)
      XFree(children);

    if (parent == root)
      break;

    window = parent;
    result = _screen->findTopLevelWindow(window);
  }

  if (result)
  {
    if (!(result->type() & (CompWindowTypeUtilMask |
                            CompWindowTypeNormalMask |
                            CompWindowTypeDialogMask |
                            CompWindowTypeModalDialogMask)))
      result = 0;
  }

  return result;
}

void
GestureEngine::OnDragStart(GeisAdapter::GeisDragData* data)
{
  if (data->touches == 3)
  {
    _drag_window = FindCompWindow(data->window);


    if (!_drag_window)
      return;

    if (!(_drag_window->actions() & CompWindowActionMoveMask))
    {
      _drag_window = 0;
      return;
    }

    if (_drag_window->state() & MAXIMIZE_STATE)
    {
      _drag_window = 0;
      return;
    }

    if (_drag_grab)
      _screen->removeGrab(_drag_grab, NULL);
    _drag_id = data->id;
    _drag_grab = _screen->pushGrab(_screen->invisibleCursor(), "unity");
  }
}
void
GestureEngine::OnDragUpdate(GeisAdapter::GeisDragData* data)
{
  if (_drag_id == data->id && _drag_window)
  {
    _screen->warpPointer((int) data->delta_x, (int) data->delta_y);
    _drag_window->move((int) data->delta_x, (int) data->delta_y, false);
  }
}

void
GestureEngine::OnDragFinish(GeisAdapter::GeisDragData* data)
{
  if (_drag_id == data->id && _drag_window)
  {
    _drag_window->syncPosition();
    EndDrag();
  }
}

void
GestureEngine::EndDrag()
{
  if (_drag_id && _drag_window)
  {
    _screen->removeGrab(_drag_grab, NULL);
    _drag_grab = 0;
    _drag_window = 0;
    _drag_id = 0;
  }
}

void
GestureEngine::OnRotateStart(GeisAdapter::GeisRotateData* data)
{

}
void
GestureEngine::OnRotateUpdate(GeisAdapter::GeisRotateData* data)
{

}
void
GestureEngine::OnRotateFinish(GeisAdapter::GeisRotateData* data)
{

}

void
GestureEngine::OnTouchStart(GeisAdapter::GeisTouchData* data)
{
  if (data->touches == 3)
  {
    CompWindow* result = FindCompWindow(data->window);

    if (result)
    {
      PluginAdapter::Default()->ShowGrabHandles(result, false);
      _touch_id = data->id;
      _touch_window = result;
    }
  }
}

void
GestureEngine::OnTouchUpdate(GeisAdapter::GeisTouchData* data)
{

}

void
GestureEngine::OnTouchFinish(GeisAdapter::GeisTouchData* data)
{
  if (_touch_id == data->id)
  {
    if (_touch_window)
      PluginAdapter::Default()->ShowGrabHandles(_touch_window, true);
    _touch_id = 0;
    _touch_window = 0;
  }
}

void
GestureEngine::OnPinchStart(GeisAdapter::GeisPinchData* data)
{
  if (data->touches == 3)
  {
    _pinch_window = FindCompWindow(data->window);

    if (!_pinch_window)
      return;

    _pinch_id = data->id;
    _pinch_start_radius = data->radius;

    if (_pinch_grab)
      _screen->removeGrab(_pinch_grab, NULL);
    _pinch_grab = _screen->pushGrab(_screen->invisibleCursor(), "unity");
  }
}
void
GestureEngine::OnPinchUpdate(GeisAdapter::GeisPinchData* data)
{
  if (data->id != _pinch_id)
    return;

  float delta_radius = data->radius - _pinch_start_radius;
  if (delta_radius > 110.0f)
  {
    _pinch_window->maximize(MAXIMIZE_STATE);
    _pinch_start_radius = data->radius;
    EndDrag();
  }
  else if (delta_radius < -110.0f)
  {
    _pinch_window->maximize(0);
    _pinch_start_radius = data->radius;
    EndDrag();
  }
}
void
GestureEngine::OnPinchFinish(GeisAdapter::GeisPinchData* data)
{
  if (_pinch_id == data->id && _pinch_window)
  {
    _screen->removeGrab(_pinch_grab, NULL);
    _pinch_grab = 0;
    _pinch_id = 0;
  }
}
