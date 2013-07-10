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
*
* Authored by: Jason Smith <jason.smith@canonical.com>
*              Brandon Schaefer <brandon.schaefer@canonical.com>
*
*/

#include <unistd.h>
#include <stdlib.h>

#include "PointerBarrier.h"

namespace unity
{
namespace ui
{

PointerBarrierWrapper::PointerBarrierWrapper()
  : active(false)
  , released(false)
  , release_once(false)
  , smoothing(75)
  , max_velocity_multiplier(1.0f)
  , direction(BOTH)
  , xi2_opcode_(0)
  , last_event_(0)
  , current_device_(0)
  , first_event_(false)
  , barrier_(0)
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

  barrier_ = XFixesCreatePointerBarrier(dpy,
                                        DefaultRootWindow(dpy),
                                        x1, y1,
                                        x2, y2,
                                        static_cast<int>(direction),
                                        0, NULL);

  active = true;
}

void PointerBarrierWrapper::DestroyBarrier()
{
  if (!active)
    return;

  active = false;

  Display *dpy = nux::GetGraphicsDisplay()->GetX11Display();
  XFixesDestroyPointerBarrier(dpy, barrier_);
}

void PointerBarrierWrapper::ReleaseBarrier(int event_id)
{
  Display *dpy = nux::GetGraphicsDisplay()->GetX11Display();
  XIBarrierReleasePointer(dpy, current_device_, barrier_, event_id);
}

void PointerBarrierWrapper::EmitCurrentData(int event_id, int x, int y)
{
  if (smoothing_count_ <= 0)
    return;

  int velocity = std::min<int>(600 * max_velocity_multiplier, smoothing_accum_ / smoothing_count_);
  SendBarrierEvent(x, y, velocity, event_id);

  smoothing_accum_ = 0;
  smoothing_count_ = 0;
}

void PointerBarrierWrapper::SendBarrierEvent(int x, int y, int velocity, int event_id)
{
  auto event = std::make_shared<BarrierEvent>(x, y, velocity, event_id);

  barrier_event.emit(this, event);
}

bool PointerBarrierWrapper::IsFirstEvent() const
{
  return first_event_;
}

int GetEventVelocity(XIBarrierEvent* event)
{
  double dx, dy;
  double speed;
  unsigned int millis;

  dx = event->dx;
  dy = event->dy;

  millis = event->dtime;

  // Sometimes dtime is 0, if so we don't want to divide by zero!
  if (!millis)
    return 1;

  speed = sqrt(dx * dx + dy * dy) / millis * 1000;

  return speed;
}

bool PointerBarrierWrapper::OwnsBarrierEvent(PointerBarrier const barrier) const
{
  return barrier_ == barrier;
}

bool PointerBarrierWrapper::HandleBarrierEvent(XIBarrierEvent* barrier_event)
{
  int velocity = GetEventVelocity(barrier_event);
  smoothing_accum_ += velocity;
  smoothing_count_++;

  current_device_ = barrier_event->deviceid;

  if (velocity > threshold)
  {
    smoothing_timeout_.reset();
    ReleaseBarrier(barrier_event->eventid);
  }
  else if (released)
  {
     /* If the barrier is released, just emit the current event without
     * waiting, so there won't be any delay on releasing the barrier. */
    smoothing_timeout_.reset();

    SendBarrierEvent(barrier_event->root_x, barrier_event->root_y,
                     velocity, barrier_event->eventid);
  }
  else if (!smoothing_timeout_)
  {
    int x = barrier_event->root_x;
    int y = barrier_event->root_y;
    int event = barrier_event->eventid;

    // If we are a new event, don't delay sending the first event
    if (last_event_ != event)
    {
      first_event_ = true;
      last_event_ = event;

      SendBarrierEvent(barrier_event->root_x, barrier_event->root_y,
                       velocity, barrier_event->eventid);

      first_event_ = false;
    }

    smoothing_timeout_.reset(new glib::Timeout(smoothing, [this, event, x, y] () {
      EmitCurrentData(event, x, y);

      smoothing_timeout_.reset();
      return false;
    }));
  }

  return true;
}

}
}
