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
#include <UnityCore/GLibSource.h>

namespace unity
{
namespace ui
{

class BarrierEvent
{
public:
  typedef std::shared_ptr<BarrierEvent> Ptr;

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
  RIGHT = 4,
};

class PointerBarrierWrapper : public sigc::trackable
{
public:
  typedef std::shared_ptr<PointerBarrierWrapper> Ptr;

  nux::Property<int> x1;
  nux::Property<int> x2;
  nux::Property<int> y1;
  nux::Property<int> y2;

  nux::Property<int> threshold;

  nux::Property<bool> active;

  nux::Property<int> smoothing;

  nux::Property<float> max_velocity_multiplier;

  nux::Property<int> index;

  nux::Property<BarrierDirection> direction;

  PointerBarrierWrapper();
  ~PointerBarrierWrapper();

  void ConstructBarrier();
  void DestroyBarrier();
  void ReleaseBarrier(int event_id);

  sigc::signal<void, PointerBarrierWrapper*, BarrierEvent::Ptr> barrier_event;

private:
  void EmitCurrentData();
  bool HandleEvent (XEvent event);
  static bool HandleEventWrapper(XEvent event, void* data);

  int last_event_;
  int last_x_;
  int last_y_;

  int event_base_;
  int error_base_;
  PointerBarrier barrier;
  
  int smoothing_count_;
  int smoothing_accum_;
  glib::Source::UniquePtr smoothing_timeout_;
};

}
}

#endif
