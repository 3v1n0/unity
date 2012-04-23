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
  _drag_window = 0;
  _pinch_id = 0;
  _touch_id = 0;
  _drag_grab = 0;
  _pinch_grab = 0;
  _fleur_cursor = XCreateFontCursor (screen->dpy (), XC_fleur);

  GeisAdapter& adapter = GeisAdapter::Instance();

  adapter.tap.connect(sigc::mem_fun(this, &GestureEngine::OnTap));

  adapter.drag_start.connect(sigc::mem_fun(this, &GestureEngine::OnDragStart));
  adapter.drag_update.connect(sigc::mem_fun(this, &GestureEngine::OnDragUpdate));
  adapter.drag_finish.connect(sigc::mem_fun(this, &GestureEngine::OnDragFinish));

  adapter.rotate_start.connect(sigc::mem_fun(this, &GestureEngine::OnRotateStart));
  adapter.rotate_update.connect(sigc::mem_fun(this, &GestureEngine::OnRotateUpdate));
  adapter.rotate_finish.connect(sigc::mem_fun(this, &GestureEngine::OnRotateFinish));

  adapter.pinch_start.connect(sigc::mem_fun(this, &GestureEngine::OnPinchStart));
  adapter.pinch_update.connect(sigc::mem_fun(this, &GestureEngine::OnPinchUpdate));
  adapter.pinch_finish.connect(sigc::mem_fun(this, &GestureEngine::OnPinchFinish));

  adapter.touch_start.connect(sigc::mem_fun(this, &GestureEngine::OnTouchStart));
  adapter.touch_update.connect(sigc::mem_fun(this, &GestureEngine::OnTouchUpdate));
  adapter.touch_finish.connect(sigc::mem_fun(this, &GestureEngine::OnTouchFinish));
}

GestureEngine::~GestureEngine()
{
		if (_fleur_cursor)
	XFreeCursor (screen->dpy (), _fleur_cursor);
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

CompWindow* GestureEngine::FindCompWindowAtPos(float fpos_x, float fpos_y)
{
  const CompWindowVector& client_list_stacking = _screen->clientList(true);

  int pos_x = fpos_x;
  int pos_y = fpos_y;

  for (auto iter = client_list_stacking.rbegin(),
       end = client_list_stacking.rend();
       iter != end; ++iter)
  {
    CompWindow* window = *iter;

    if (pos_x >= window->x() && pos_x <= (window->width() + window->x())
        &&
        pos_y >= window->y() && pos_y <= (window->height() + window->y()))
      return window;
  }

  return nullptr;
}

void
GestureEngine::OnDragStart(GeisAdapter::GeisDragData* data)
{
  if (data->touches == 3)
  {
    _drag_window = FindCompWindowAtPos(data->focus_x, data->focus_y);


    if (!_drag_window)
      return;

    if (!(_drag_window->actions() & CompWindowActionMoveMask))
    {
      _drag_window = 0;
      return;
    }

    /* Don't allow windows to be dragged if completely maximized */
    if ((_drag_window->state() & MAXIMIZE_STATE) == MAXIMIZE_STATE)
    {
      _drag_window = 0;
      return;
    }

    if (_drag_grab)
      _screen->removeGrab(_drag_grab, NULL);
    _drag_id = data->id;
    _drag_grab = _screen->pushGrab(_fleur_cursor, "unity");
    _drag_window->grabNotify (_drag_window->serverGeometry ().x (),
                              _drag_window->serverGeometry ().y (),
                              0, CompWindowGrabMoveMask | CompWindowGrabButtonMask);
  }
}

/* FIXME: CompScreen::warpPointer filters out motion events which
 * other plugins may need to process, but for most cases in core
 * they should be filtered out. */
void
GestureEngine::OnDragUpdate(GeisAdapter::GeisDragData* data)
{
  if (_drag_id == data->id && _drag_window)
  {
    unsigned int px = std::max (std::min (pointerX + static_cast <int> (data->delta_x), screen->width ()), 0);
    unsigned int py = std::max (std::min (pointerY + static_cast <int> (data->delta_y), screen->height ()), 0);

    if (_drag_window->state () & CompWindowStateMaximizedVertMask)
      py = pointerY;
    if (_drag_window->state () & CompWindowStateMaximizedHorzMask)
      px = pointerX;

    XWarpPointer(screen->dpy (),
     None, screen->root (),
     0, 0, 0, 0,
     px, py);

    XSync(screen->dpy (), false);
    _drag_window->move(px - pointerX, py - pointerY, false);

    pointerX = px;
    pointerY = py;
  }
}

void
GestureEngine::OnDragFinish(GeisAdapter::GeisDragData* data)
{
  if (_drag_id == data->id && _drag_window)
  {
    _drag_window->ungrabNotify ();
    _drag_window->syncPosition();
    EndDrag();
  }
}

void
GestureEngine::EndDrag()
{
  if (_drag_window)
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
  if (data->touches == 3 && data->window != 0)
  {
    CompWindow* result = FindCompWindowAtPos(data->focus_x, data->focus_y);

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
    _pinch_window = FindCompWindowAtPos(data->focus_x, data->focus_y);

    if (!_pinch_window)
      return;

    _pinch_id = data->id;

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

  if (data->radius > 1.25)
  {
    _pinch_window->maximize(MAXIMIZE_STATE);
    EndDrag();
  }
  else if (data->radius < 0.8)
  {
    _pinch_window->maximize(0);
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
