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

struct EdgeBarrierSubscriber
{
  enum class Result
  {
    IGNORED,
    HANDLED,
    ALREADY_HANDLED,
    NEEDS_RELEASE
  };

  virtual Result HandleBarrierEvent(PointerBarrierWrapper* owner, BarrierEvent::Ptr event) = 0;
};

class EdgeBarrierController : public sigc::trackable
{
public:
  typedef std::shared_ptr<EdgeBarrierController> Ptr;

  EdgeBarrierController();
  ~EdgeBarrierController();

  nux::RWProperty<bool> sticky_edges;
  nux::Property<launcher::Options::Ptr> options;

  void AddHorizontalSubscriber(EdgeBarrierSubscriber* subscriber, unsigned int monitor);
  void RemoveHorizontalSubscriber(EdgeBarrierSubscriber* subscriber, unsigned int monitor);

  void AddVerticalSubscriber(EdgeBarrierSubscriber* subscriber, unsigned int monitor);
  void RemoveVerticalSubscriber(EdgeBarrierSubscriber* subscriber, unsigned int monitor);

  EdgeBarrierSubscriber* GetHorizontalSubscriber(unsigned int monitor);
  EdgeBarrierSubscriber* GetVerticalSubscriber(unsigned int monitor);

private:
  struct Impl;
  std::unique_ptr<Impl> pimpl;
  friend class TestEdgeBarrierController;
};

}
}

#endif
