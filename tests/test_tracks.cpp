// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2013 Canonical Ltd
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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 */

#include <gtest/gtest.h>
#include <glib-object.h>

#include <UnityCore/GLibWrapper.h>
#include <UnityCore/Tracks.h>

#include "test_utils.h"

using namespace std;
using namespace unity::dash;

namespace
{
static const string swarm_name = "com.canonical.test.tracks";
static const unsigned int n_rows = 5;

static void WaitForSynchronize(Tracks& model)
{
  Utils::WaitUntil([&model] { return model.count == n_rows; });
}
}

TEST(TestTracks, TestConstruction)
{
  Tracks model;
  model.swarm_name = swarm_name;
}

TEST(TestTracks, TestSynchronization)
{
  Tracks model;
  model.swarm_name = swarm_name;

  WaitForSynchronize(model);
  EXPECT_EQ(model.count, n_rows);
}

// We're testing the model's ability to store and retrieve random pointers
TEST(TestTracks, TestOnRowChanged)
{
  Tracks model;
  model.swarm_name = swarm_name;
  WaitForSynchronize(model);

  bool changed = false;
  model.track_changed.connect([&changed] (Track const&) { changed = true;});
  Utils::WaitUntil(changed, 2, "Did not detect track change from "+swarm_name+".");
}


// We're testing the model's ability to store and retrieve random pointers
TEST(TestTracks, TestOnRowAdded)
{
  Tracks model;
  model.swarm_name = swarm_name;
  WaitForSynchronize(model);

  bool added = false;
  model.track_added.connect([&added] (Track const&) { added = true;});
  Utils::WaitUntil(added, 2, "Did not detect track add "+swarm_name+".");
}

// We're testing the model's ability to store and retrieve random pointers
TEST(TestTracks, TestOnRowRemoved)
{
  Tracks model;
  model.swarm_name = swarm_name;
  WaitForSynchronize(model);

  bool removed = false;
  model.track_removed.connect([&removed] (Track const&) { removed = true;});
  Utils::WaitUntil(removed, 2, "Did not detect track removal "+swarm_name+".");
}

TEST(TestTracks, TestTrackCopy)
{
  Tracks model;
  model.swarm_name = swarm_name;
  WaitForSynchronize(model);
  
  Track track = model.RowAtIndex(0);
  Track track_2(track);

  EXPECT_EQ(track.uri(), track_2.uri());
  EXPECT_EQ(track.track_number(), track_2.track_number());
  EXPECT_EQ(track.title(), track_2.title());
  EXPECT_EQ(track.length(), track_2.length());
  EXPECT_EQ(track.index(), track_2.index());
}

TEST(TestTracks, TestTrackEqual)
{
  Tracks model;
  model.swarm_name = swarm_name;
  WaitForSynchronize(model);

  Track track = model.RowAtIndex(0);
  Track track_2(NULL, NULL, NULL);
  track_2 = track;

  EXPECT_EQ(track.uri(), track_2.uri());
  EXPECT_EQ(track.track_number(), track_2.track_number());
  EXPECT_EQ(track.title(), track_2.title());
  EXPECT_EQ(track.length(), track_2.length());
  EXPECT_EQ(track.index(), track_2.index());
}
