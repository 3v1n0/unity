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

namespace local
{
namespace
{
  bool is_selected_for = false;
}
}

PointerBarrierWrapper::PointerBarrierWrapper()
  : active(false)
  , released(false)
  , smoothing(75)
  , max_velocity_multiplier(1.0f)
  , direction(BOTH)
  , smoothing_count_(0)
  , smoothing_accum_(0)
{}

PointerBarrierWrapper::~PointerBarrierWrapper()
{
  DestroyBarrier();
}

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
                                               static_cast<int>(direction),
                                               threshold,
                                               0,
                                               NULL);

  if (!local::is_selected_for)
  {
    XFixesSelectBarrierInput(dpy, DefaultRootWindow(dpy), 0xdeadbeef);
    local::is_selected_for = true;
  }

  active = true;

  nux::GraphicsDisplay::EventFilterArg event_filter;
  event_filter.filter = &PointerBarrierWrapper::HandleEventWrapper;
  event_filter.data = this;

  nux::GetGraphicsDisplay()->AddEventFilter(event_filter);
}

void PointerBarrierWrapper::DestroyBarrier()
{
  if (!active)
    return;

  active = false;

  Display *dpy = nux::GetGraphicsDisplay()->GetX11Display();
  XFixesDestroyPointerBarrier(dpy, barrier);

  nux::GetGraphicsDisplay()->RemoveEventFilter(this);
}

void PointerBarrierWrapper::ReleaseBarrier(int event_id)
{
  XFixesBarrierReleasePointer(nux::GetGraphicsDisplay()->GetX11Display(), barrier, event_id);
}

void PointerBarrierWrapper::EmitCurrentData(int event_id, int x, int y)
{
  if (smoothing_count_ <= 0)
    return;

  int velocity = std::min<int>(600 * max_velocity_multiplier, smoothing_accum_ / smoothing_count_);
  auto event = std::make_shared<BarrierEvent>(x, y, velocity, event_id);

  barrier_event.emit(this, event);

  smoothing_accum_ = 0;
  smoothing_count_ = 0;
}

bool PointerBarrierWrapper::HandleEvent(XEvent xevent)
{
  if(xevent.type - event_base_ == XFixesBarrierNotify)
  {
    XFixesBarrierNotifyEvent *notify_event = (XFixesBarrierNotifyEvent *)&xevent;

    if (notify_event->barrier == barrier && notify_event->subtype == XFixesBarrierHitNotify)
    {
      smoothing_accum_ += notify_event->velocity;
      smoothing_count_++;

      if (released)
      {
        /* If the barrier is released, just emit the current event without
         * waiting, so there won't be any delay on releasing the barrier. */
        smoothing_timeout_.reset();
        auto event = std::make_shared<BarrierEvent>(notify_event->x, notify_event->y,
                                                    notify_event->velocity, notify_event->event_id);
        barrier_event.emit(this, event);
      }
      else if (!smoothing_timeout_)
      {
        int x = notify_event->x;
        int y = notify_event->y;
        int event = notify_event->event_id;

        auto smoothing_cb = [&, event, x, y] ()
        {
          EmitCurrentData(event, x, y);

          smoothing_timeout_.reset();
          return false;
        };

        smoothing_timeout_.reset(new glib::Timeout(smoothing, smoothing_cb));
      }
    }

    return notify_event->barrier == barrier;
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
