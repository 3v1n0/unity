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
 *              Brandon Schaefer <brandon.schaefer@canonical.com>
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
  bool HandleBarrierEvent(XIBarrierEvent* b_ev) { return PointerBarrierWrapper::HandleBarrierEvent(b_ev); }
};

XIBarrierEvent GetGenericEvent (unsigned int id)
{
  XIBarrierEvent ev;

  ev.evtype = GenericEvent;
  ev.barrier = 0;
  ev.eventid = id;
  ev.root_x = 555;
  ev.root_y = 333;
  ev.dx = 10;
  ev.dy = 10;
  ev.dtime = 15;

  return ev;
}

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

TEST(TestPointerBarrier, HandleHitNotifyEvents)
{
  MockPointerBarrier pb;
  pb.threshold = 1000;
  XIBarrierEvent ev = GetGenericEvent(0xdeadbeef);

  bool got_event = false;

  pb.barrier_event.connect([&] (PointerBarrierWrapper* pbw, BarrierEvent::Ptr bev) {
    if (!pbw->IsFirstEvent())
    {
      got_event = true;

      EXPECT_EQ(pbw, &pb);
      EXPECT_EQ(bev->x, ev.root_x);
      EXPECT_EQ(bev->y, ev.root_y);
      EXPECT_EQ(bev->velocity, 600 * pb.max_velocity_multiplier);
      EXPECT_EQ(bev->event_id, ev.eventid);
     }
  });

  EXPECT_TRUE(pb.HandleBarrierEvent(&ev));
  EXPECT_FALSE(got_event);

  Utils::WaitForTimeoutMSec(pb.smoothing());

  EXPECT_TRUE(got_event);
}

TEST(TestPointerBarrier, HandleHitNotifyReleasedEvents)
{
  MockPointerBarrier pb;
  pb.threshold = 1000;
  XIBarrierEvent ev = GetGenericEvent(0xabba);
  bool got_event = false;

  pb.barrier_event.connect([&] (PointerBarrierWrapper* pbw, BarrierEvent::Ptr bev) {
    got_event = true;

    EXPECT_EQ(pbw, &pb);
    EXPECT_EQ(bev->x, ev.root_x);
    EXPECT_EQ(bev->y, ev.root_y);
    EXPECT_GT(bev->velocity, 0);
    EXPECT_EQ(bev->event_id, ev.eventid);
  });

  pb.released = true;
  EXPECT_TRUE(pb.HandleBarrierEvent(&ev));
  EXPECT_TRUE(got_event);
}

TEST(TestPointerBarrier, ReciveFirstEvent)
{
  MockPointerBarrier pb;
  pb.threshold = 1000;
  XIBarrierEvent ev = GetGenericEvent(0xabba);

  bool first_is_true = false;

  pb.barrier_event.connect([&] (PointerBarrierWrapper* pbw, BarrierEvent::Ptr bev) {
    first_is_true = true;
  });

  EXPECT_TRUE(pb.HandleBarrierEvent(&ev));
  EXPECT_TRUE(first_is_true);
}

TEST(TestPointerBarrier, ReciveSecondEventFirstFalse)
{
  MockPointerBarrier pb;
  pb.threshold = 1000;
  XIBarrierEvent ev = GetGenericEvent(0xabba);
  int events_recived = 0;

  pb.barrier_event.connect([&] (PointerBarrierWrapper* pbw, BarrierEvent::Ptr bev) {
      events_recived++;

      if (events_recived == 1)
        EXPECT_TRUE(pbw->IsFirstEvent());
      else
        EXPECT_FALSE(pbw->IsFirstEvent());
  });

  EXPECT_TRUE(pb.HandleBarrierEvent(&ev));

  Utils::WaitForTimeoutMSec(pb.smoothing());

  EXPECT_EQ(events_recived, 2);
}

}
