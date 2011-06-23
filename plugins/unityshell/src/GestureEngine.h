/*
 * GestureEngine.h
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

#include <core/core.h>

#include <sigc++/sigc++.h>
#include <Nux/Nux.h>
#include "GeisAdapter.h"

class GestureEngine : public sigc::trackable
{
  public:
    GestureEngine (CompScreen *screen);
    virtual ~GestureEngine ();
    
    void OnTap (GeisAdapter::GeisTapData *data);
    
    void OnDragStart (GeisAdapter::GeisDragData *data);
    void OnDragUpdate (GeisAdapter::GeisDragData *data);
    void OnDragFinish (GeisAdapter::GeisDragData *data);

    void OnRotateStart (GeisAdapter::GeisRotateData *data);
    void OnRotateUpdate (GeisAdapter::GeisRotateData *data);
    void OnRotateFinish (GeisAdapter::GeisRotateData *data);
    
    void OnPinchStart (GeisAdapter::GeisPinchData *data);
    void OnPinchUpdate (GeisAdapter::GeisPinchData *data);
    void OnPinchFinish (GeisAdapter::GeisPinchData *data);
    
    void OnTouchStart (GeisAdapter::GeisTouchData *data);
    void OnTouchUpdate (GeisAdapter::GeisTouchData *data);
    void OnTouchFinish (GeisAdapter::GeisTouchData *data);
    
    void EndDrag ();
  private:
    CompWindow * FindCompWindow (Window window);
  
    CompScreen *_screen;
    CompWindow *_drag_window;
    CompWindow *_pinch_window;
    CompWindow *_touch_window;
    CompScreen::GrabHandle _drag_grab;
    CompScreen::GrabHandle _pinch_grab;
    
    int _drag_id;
    int _pinch_id;
    int _touch_id;
    
    float _pinch_start_radius;
};
