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
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#include <gmock/gmock.h>
using namespace testing;

#include "launcher/DeltaTracker.h"

namespace unity
{

class TestDeltaTracker : public Test
{
public:
  TestDeltaTracker()
  {
  }

  DeltaTracker delta_tracker_;
};


TEST_F(TestDeltaTracker, TestDirectionEmptyOnStart)
{
  ASSERT_EQ(delta_tracker_.AmountOfDirectionsChanged(), 0);
}

TEST_F(TestDeltaTracker, TestCorrectDirections)
{
  delta_tracker_.HandleNewMouseDelta(0, -1);
  delta_tracker_.HandleNewMouseDelta(1, 0);

  ASSERT_EQ(delta_tracker_.AmountOfDirectionsChanged(), 2);
}

TEST_F(TestDeltaTracker, TestNoDuplicates)
{
  delta_tracker_.HandleNewMouseDelta(0, -1);
  delta_tracker_.HandleNewMouseDelta(0, -1);

  ASSERT_EQ(delta_tracker_.AmountOfDirectionsChanged(), 1);
}

TEST_F(TestDeltaTracker, TestAllDirections)
{
  delta_tracker_.HandleNewMouseDelta(0, -1);
  delta_tracker_.HandleNewMouseDelta(0, 1);
  delta_tracker_.HandleNewMouseDelta(-1, 0);
  delta_tracker_.HandleNewMouseDelta(1, 0);

  ASSERT_EQ(delta_tracker_.AmountOfDirectionsChanged(), 4);
}

TEST_F(TestDeltaTracker, TestResetStates)
{
  delta_tracker_.HandleNewMouseDelta(0, -1);
  delta_tracker_.HandleNewMouseDelta(0, 1);
  delta_tracker_.HandleNewMouseDelta(-1, 0);
  delta_tracker_.HandleNewMouseDelta(1, 0);

  ASSERT_EQ(delta_tracker_.AmountOfDirectionsChanged(), 4);

  delta_tracker_.ResetState();
  ASSERT_EQ(delta_tracker_.AmountOfDirectionsChanged(), 0);
}

}
