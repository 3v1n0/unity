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

  void ResizeBarrierList(std::vector<nux::Geometry> const& layout);
  void SetupBarriers(std::vector<nux::Geometry> const& layout);

  void OnPointerBarrierEvent(PointerBarrierWrapper* owner, BarrierEvent::Ptr const& event);
  void BarrierPush(PointerBarrierWrapper* owner, BarrierEvent::Ptr const& event);
  void BarrierRelease(PointerBarrierWrapper* owner, int event);
  void BarrierReset();

  std::vector<PointerBarrierWrapper::Ptr> barriers_;
  std::vector<EdgeBarrierSubscriber*> subscribers_;
  Decaymulator decaymulator_;
  glib::Source::UniquePtr release_timeout_;
  float edge_overcome_pressure_;
  EdgeBarrierController* parent_;
};

} //namespace ui
} //namespace unity


#endif // EDGE_BARRIER_CONTROLLER_IMPL_PRIVATE
