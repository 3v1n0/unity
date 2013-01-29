/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Marco Trevisan (Treviño) <marco.trevisan@canonical.com>
 *
 */

#include <gtest/gtest.h>
#include <limits>

#include "test_utils.h"
#include "PointerBarrier.h"

using namespace unity::ui;

namespace
{

class MockPointerBarrier : public PointerBarrierWrapper
{
public:
  bool HandleEvent(XEvent ev) { return PointerBarrierWrapper::HandleEvent(ev); }
};

TEST(TestPointerBarrier, Construction)
{
  PointerBarrierWrapper pb;

  EXPECT_EQ(pb.active, false);
  EXPECT_EQ(pb.released, false);
  EXPECT_EQ(pb.smoothing, 75);
  EXPECT_EQ(pb.max_velocity_multiplier, 1.0f);
  EXPECT_EQ(pb.direction, BOTH);
}

TEST(TestPointerBarrier, EventConstruction)
{
  BarrierEvent bev(1, 2, 3, 4);
  EXPECT_EQ(bev.x, 1);
  EXPECT_EQ(bev.y, 2);
  EXPECT_EQ(bev.velocity, 3);
  EXPECT_EQ(bev.event_id, 4);
}

TEST(TestPointerBarrier, HandleInvalidEvents)
{
  MockPointerBarrier pb;
  XFixesBarrierNotifyEvent ev;
  auto xev = reinterpret_cast<XEvent*>(&ev);

  ev.type = XFixesBarrierNotify + 1;
  EXPECT_FALSE(pb.HandleEvent(*xev));

  ev.type = XFixesBarrierNotify;
  ev.subtype = XFixesBarrierHitNotify + 1;
  ev.barrier = 1;
  EXPECT_FALSE(pb.HandleEvent(*xev));

  ev.barrier = 0;
  EXPECT_TRUE(pb.HandleEvent(*xev));
}

TEST(TestPointerBarrier, HandleHitNotifyEvents)
{
  MockPointerBarrier pb;
  XFixesBarrierNotifyEvent ev;
  auto xev = reinterpret_cast<XEvent*>(&ev);

  ev.type = XFixesBarrierNotify;
  ev.subtype = XFixesBarrierHitNotify;
  ev.barrier = 0;
  ev.event_id = 0xdeadbeef;
  ev.x = 555;
  ev.y = 333;
  ev.velocity = std::numeric_limits<int>::max();

  bool got_event = false;

  pb.barrier_event.connect([&] (PointerBarrierWrapper* pbw, BarrierEvent::Ptr bev) {
    got_event = true;

    EXPECT_EQ(pbw, &pb);
    EXPECT_EQ(bev->x, ev.x);
    EXPECT_EQ(bev->y, ev.y);
    EXPECT_EQ(bev->velocity, 600 * pb.max_velocity_multiplier);
    EXPECT_EQ(bev->event_id, ev.event_id);
  });

  EXPECT_TRUE(pb.HandleEvent(*xev));
  EXPECT_FALSE(got_event);

  Utils::WaitForTimeoutMSec(pb.smoothing());

  EXPECT_TRUE(got_event);
}

TEST(TestPointerBarrier, HandleHitNotifyReleasedEvents)
{
  MockPointerBarrier pb;
  XFixesBarrierNotifyEvent ev;
  auto xev = reinterpret_cast<XEvent*>(&ev);

  ev.type = XFixesBarrierNotify;
  ev.subtype = XFixesBarrierHitNotify;
  ev.barrier = 0;
  ev.event_id = 0xabba;
  ev.x = 3333;
  ev.y = 5555;
  ev.velocity = std::numeric_limits<int>::max();

  bool got_event = false;

  pb.barrier_event.connect([&] (PointerBarrierWrapper* pbw, BarrierEvent::Ptr bev) {
    got_event = true;

    EXPECT_EQ(pbw, &pb);
    EXPECT_EQ(bev->x, ev.x);
    EXPECT_EQ(bev->y, ev.y);
    EXPECT_EQ(bev->velocity, ev.velocity);
    EXPECT_EQ(bev->event_id, ev.event_id);
  });

  pb.released = true;
  EXPECT_TRUE(pb.HandleEvent(*xev));
  EXPECT_TRUE(got_event);
}

}
