// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012-2013 Canonical Ltd
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
 *              Marco Trevisan <marco.trevisan@canonical.com>
 */

#ifndef EDGE_BARRIER_CONTROLLER_IMPL_PRIVATE
#define EDGE_BARRIER_CONTROLLER_IMPL_PRIVATE

#include "Decaymulator.h"
#include "UnityCore/GLibSource.h"

namespace unity
{
namespace ui
{

// NOTE: This private header is not part of the public interface
struct EdgeBarrierController::Impl
{
  Impl(EdgeBarrierController *parent);
  ~Impl();

  void AddSubscriber(EdgeBarrierSubscriber* subscriber, unsigned int monitor, std::vector<EdgeBarrierSubscriber*>& subscribers);
  void RemoveSubscriber(EdgeBarrierSubscriber* subscriber, unsigned int monitor, std::vector<EdgeBarrierSubscriber*>& subscribers);

  void ResizeBarrierList(std::vector<nux::Geometry> const& layout);
  void SetupBarriers(std::vector<nux::Geometry> const& layout);

  void OnPointerBarrierEvent(PointerBarrierWrapper* owner, BarrierEvent::Ptr const& event);
  void BarrierPush(PointerBarrierWrapper* owner, BarrierEvent::Ptr const& event);
  void BarrierRelease(PointerBarrierWrapper* owner, int event);
  void BarrierReset();

  bool EventIsInsideYBreakZone(BarrierEvent::Ptr const& event);
  bool EventIsInsideXBreakZone(BarrierEvent::Ptr const& event);

  void AddEventFilter();

  PointerBarrierWrapper::Ptr FindBarrierEventOwner(XIBarrierEvent* barrier_event);

  static bool HandleEventCB(XEvent event, void* data);
  bool HandleEvent(XEvent event);

  std::vector<PointerBarrierWrapper::Ptr> vertical_barriers_;
  std::vector<PointerBarrierWrapper::Ptr> horizontal_barriers_;

  std::vector<EdgeBarrierSubscriber*> vertical_subscribers_;
  std::vector<EdgeBarrierSubscriber*> horizontal_subscribers_;

  Decaymulator decaymulator_;
  glib::Source::UniquePtr release_timeout_;
  int xi2_opcode_;
  float edge_overcome_pressure_;
  EdgeBarrierController* parent_;
};

} //namespace ui
} //namespace unity


#endif // EDGE_BARRIER_CONTROLLER_IMPL_PRIVATE
