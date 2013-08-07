// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010 Canonical Ltd
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
 */

#ifndef UNITY_POINTERWRAPPER_H
#define UNITY_POINTERWRAPPER_H

#include <Nux/Nux.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/XInput2.h>
#include <UnityCore/GLibSource.h>

namespace unity
{
namespace ui
{

struct BarrierEvent
{
  typedef std::shared_ptr<BarrierEvent> Ptr;

  BarrierEvent(int x_, int y_, int velocity_, int event_id_)
    : x(x_), y(y_), velocity(velocity_), event_id(event_id_)
  {}

  int x;
  int y;
  int velocity;
  int event_id;
};


// values picked to match Xfixes values
enum BarrierDirection
{
  BOTH = 0,
  LEFT = 1,
  UP = 2, // FIXME
  RIGHT = 4,
  DOWN =  8 // FIXME
};

enum BarrierOrientation
{
  VERTICAL = 0,
  HORIZONTAL
};

class PointerBarrierWrapper : public sigc::trackable
{
public:
  typedef std::shared_ptr<PointerBarrierWrapper> Ptr;

  PointerBarrierWrapper();
  virtual ~PointerBarrierWrapper();

  nux::Property<int> x1;
  nux::Property<int> x2;
  nux::Property<int> y1;
  nux::Property<int> y2;

  nux::Property<int> threshold;

  nux::Property<bool> active;
  nux::Property<bool> released;
  nux::Property<bool> release_once;

  nux::Property<int> smoothing;

  nux::Property<float> max_velocity_multiplier;

  nux::Property<int> index;

  nux::Property<BarrierDirection> direction;
  nux::Property<BarrierOrientation> orientation;

  virtual void ConstructBarrier();
  virtual void DestroyBarrier();
  virtual void ReleaseBarrier(int event_id);

  sigc::signal<void, PointerBarrierWrapper*, BarrierEvent::Ptr> barrier_event;

  bool IsFirstEvent() const;

  bool OwnsBarrierEvent(PointerBarrier const barrier) const;
  bool HandleBarrierEvent(XIBarrierEvent* barrier_event);

protected:
  void EmitCurrentData(int event_id, int x, int y);

private:
  void SendBarrierEvent(int x, int y, int velocity, int event_id);

  int xi2_opcode_;
  int last_event_;
  int current_device_;
  bool first_event_;
  PointerBarrier barrier_;

  int smoothing_count_;
  int smoothing_accum_;
  glib::Source::UniquePtr smoothing_timeout_;
};

}
}

#endif
