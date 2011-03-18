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
 
#include "GestureEngine.h"

GestureEngine::GestureEngine (CompScreen *screen)
{
  _screen = screen;
  _pinch_grab = 0;
  _drag_grab = 0;
  
  GeisAdapter *adapter = GeisAdapter::Default (screen);
  
  adapter->tap.connect (sigc::mem_fun (this, &GestureEngine::OnTap));
  
  adapter->drag_start.connect (sigc::mem_fun (this, &GestureEngine::OnDragStart));
  adapter->drag_update.connect (sigc::mem_fun (this, &GestureEngine::OnDragUpdate));
  adapter->drag_finish.connect (sigc::mem_fun (this, &GestureEngine::OnDragFinish));
  
  adapter->rotate_start.connect (sigc::mem_fun (this, &GestureEngine::OnRotateStart));
  adapter->rotate_update.connect (sigc::mem_fun (this, &GestureEngine::OnRotateUpdate));
  adapter->rotate_finish.connect (sigc::mem_fun (this, &GestureEngine::OnRotateFinish));
  
  adapter->pinch_start.connect (sigc::mem_fun (this, &GestureEngine::OnPinchStart));
  adapter->pinch_update.connect (sigc::mem_fun (this, &GestureEngine::OnPinchUpdate));
  adapter->pinch_finish.connect (sigc::mem_fun (this, &GestureEngine::OnPinchFinish));
}

GestureEngine::~GestureEngine ()
{

}

void
GestureEngine::OnTap (GeisAdapter::GeisTapData *data)
{

}

void
GestureEngine::OnDragStart (GeisAdapter::GeisDragData *data)
{
  if (data->touches == 3)
  {
    _drag_window = _screen->findWindow (_screen->activeWindow ());
    
    if (!_drag_window)
      return;
    
    if (_drag_window->state () & MAXIMIZE_STATE)
    {
      _drag_window = 0;
      return;
    }
    
    if (_drag_grab)
      _screen->removeGrab (_drag_grab, NULL);
    _drag_grab = _screen->pushGrab (_screen->invisibleCursor (), "unity");
  }
}
void
GestureEngine::OnDragUpdate (GeisAdapter::GeisDragData *data)
{
  if (data->touches == 3 && _drag_window)
  {
    _screen->warpPointer ((int) data->delta_x, (int) data->delta_y);
    _drag_window->move ((int) data->delta_x, (int) data->delta_y, false);
  }
}

void
GestureEngine::OnDragFinish (GeisAdapter::GeisDragData *data)
{
  if (_drag_window)
  {
    _drag_window->syncPosition ();
    RejectDrag ();
  }
}

void
GestureEngine::RejectDrag ()
{
  if (_drag_window)
  {
    _screen->removeGrab (_drag_grab, NULL);
    _drag_grab = 0;
    _drag_window = 0;
  }
}

void
GestureEngine::OnRotateStart (GeisAdapter::GeisRotateData *data)
{

}
void
GestureEngine::OnRotateUpdate (GeisAdapter::GeisRotateData *data)
{

}
void
GestureEngine::OnRotateFinish (GeisAdapter::GeisRotateData *data)
{

}

void
GestureEngine::OnPinchStart (GeisAdapter::GeisPinchData *data)
{
  if (data->touches == 3)
  {
    _pinch_window = _screen->findWindow (_screen->activeWindow ());
    
    if (!_pinch_window)
      return;
    
    _pinch_start_radius = data->radius;
    
    if (_pinch_grab)
      _screen->removeGrab (_pinch_grab, NULL);
    _pinch_grab = _screen->pushGrab (_screen->invisibleCursor (), "unity");
  }
}
void
GestureEngine::OnPinchUpdate (GeisAdapter::GeisPinchData *data)
{
  float delta_radius = data->radius - _pinch_start_radius;
  if (delta_radius > 150.0f)
  {
    _pinch_window->maximize (MAXIMIZE_STATE);
    _pinch_start_radius = data->radius;
    RejectDrag ();
  }
  else if (delta_radius < -150.0f)
  {
    _pinch_window->maximize (0);
    _pinch_start_radius = data->radius;
    RejectDrag ();
  }
}
void
GestureEngine::OnPinchFinish (GeisAdapter::GeisPinchData *data)
{
  if (_pinch_window)
  {
    _screen->removeGrab (_pinch_grab, NULL);
    _pinch_grab = 0;
  }
}
