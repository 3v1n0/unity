// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2011 Canonical Ltd
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 3 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <X11/extensions/Xfixes.h>

#include "PointerBarrier.h"

namespace unity
{
namespace ui
{
  
void PointerBarrierWrapper::ConstructBarrier()
{
  if (active)
    return;
  
  Display *dpy = nux::GetGraphicsDisplay()->GetX11Display();

  XFixesQueryExtension(dpy, &event_base_, &error_base_);
  
  int maj,min;
  XFixesQueryVersion(dpy, &maj, &min);

  barrier = XFixesCreatePointerBarrierVelocity(dpy,
                                               DefaultRootWindow(dpy),
                                               x1, y1,
                                               x2, y2,
                                               0,
                                               threshold,
                                               0,
                                               NULL);
  
  XFixesSelectBarrierInput(dpy, DefaultRootWindow(dpy), 0xdeadbeef);

  active = true;
}

void PointerBarrierWrapper::DestroyBarrier()
{
  return;
  if (!active)
    return;

  Display *dpy = nux::GetGraphicsDisplay()->GetX11Display();
  XFixesDestroyPointerBarrier(dpy, barrier);

  active = false;
}

bool PointerBarrierWrapper::HandleEvent(XEvent xevent)
{
  if(xevent.type - event_base_ == XFixesBarrierNotify)
  {
    XFixesBarrierNotifyEvent *notify_event = (XFixesBarrierNotifyEvent *)&xevent;
    //int x = notify_event->x;
    //int y = notify_event->y;
    int velocity = notify_event->velocity;
    int event_id = notify_event->event_id;

    if (velocity > 30)
      XFixesBarrierReleasePointer (nux::GetGraphicsDisplay()->GetX11Display(), notify_event->barrier, event_id);

    return true;
  }

  return false;
}

bool PointerBarrierWrapper::HandleEventWrapper(XEvent event, void* data)
{
  PointerBarrierWrapper* wrapper = (PointerBarrierWrapper*)data;
  return wrapper->HandleEvent(event);
}

}
}
