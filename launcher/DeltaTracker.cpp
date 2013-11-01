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
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#include "DeltaTracker.h"

namespace unity
{

DeltaTracker::DeltaTracker()
  : delta_state_(DeltaState::NONE)
{
}

void DeltaTracker::HandleNewMouseDelta(int dx, int dy)
{
  if (dx > 0)
  {
    delta_state_ |= DeltaState::RIGHT;
  }
  else if (dx < 0)
  {
    delta_state_ |= DeltaState::LEFT;
  }

  if (dy > 0)
  {
    delta_state_ |= DeltaState::DOWN;
  }
  else if (dy < 0)
  {
    delta_state_ |= DeltaState::UP;
  }
}

void DeltaTracker::ResetState()
{
  delta_state_ = DeltaState::NONE;
}

unsigned int DeltaTracker::AmountOfDirectionsChanged() const
{
  unsigned int directions_changed = 0;

  if (HasState(DeltaState::RIGHT))
    directions_changed++;

  if (HasState(DeltaState::LEFT))
    directions_changed++;

  if (HasState(DeltaState::UP))
    directions_changed++;

  if (HasState(DeltaState::DOWN))
    directions_changed++;

  return directions_changed;
}

bool DeltaTracker::HasState(DeltaState const& state) const
{
  return (delta_state_ & state);
}

}
