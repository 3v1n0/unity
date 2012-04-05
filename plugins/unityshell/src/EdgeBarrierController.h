// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012 Canonical Ltd
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

#ifndef UNITY_EDGEBARRIERCONTROLLER_H
#define UNITY_EDGEBARRIERCONTROLLER_H

#include "PointerBarrier.h"
#include "LauncherOptions.h"

namespace unity {
namespace ui {

class EdgeBarrierSubscriber
{
public:
  virtual bool HandleBarrierEvent(PointerBarrierWrapper* owner, BarrierEvent::Ptr event) { return false; };
};

class EdgeBarrierController : public sigc::trackable
{
public:
  typedef std::shared_ptr<EdgeBarrierController> Ptr;

  EdgeBarrierController();
  ~EdgeBarrierController();

  nux::Property<bool> sticky_edges;
  nux::Property<launcher::Options::Ptr> options;

  void Subscribe(EdgeBarrierSubscriber* subscriber, int monitor);
  void Unsubscribe(EdgeBarrierSubscriber* subscriber, int monitor);

private:
  struct Impl;
  Impl* pimpl;
};

}
}

#endif
