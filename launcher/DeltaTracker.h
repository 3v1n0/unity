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

#ifndef DELTA_TRACKER_H
#define DELTA_TRACKER_H

namespace unity
{

class DeltaTracker
{
public:
  DeltaTracker();

  void HandleNewMouseDelta(int dx, int dy);
  void ResetState();

  unsigned int AmountOfDirectionsChanged() const;

private:
  enum DeltaState
  {
    NONE  = 1 << 0,
    RIGHT = 1 << 1,
    DOWN  = 1 << 2,
    LEFT  = 1 << 3,
    UP    = 1 << 4
  };

  bool HasState(DeltaState const& state) const;

  unsigned int delta_state_;
};

}

#endif // DELTA_TRACKER_H
