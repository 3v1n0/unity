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

#include <gmock/gmock.h>
#include "test_utils.h"
#include "test_uscreen_mock.h"

#include "EdgeBarrierController.h"

using namespace unity;
using namespace unity::ui;
using namespace testing;

namespace
{

class MockPointerBarrier : public PointerBarrierWrapper
{
public:
  MockPointerBarrier(int monitor = 0, bool released_ = false)
  {
    index = monitor;
    x1 = 1;
    released = released_;
  }

  MOCK_METHOD0(ConstructBarrier, void());
  MOCK_METHOD0(DestroyBarrier, void());
  MOCK_METHOD1(ReleaseBarrier, void(int));
};

class MockEdgeBarrierController : public EdgeBarrierController
{
public:
  void ProcessBarrierEvent(PointerBarrierWrapper* owner, BarrierEvent::Ptr event)
  {
    EdgeBarrierController::ProcessBarrierEvent(owner, event);
  }
};

class TestBarrierSubscriber : public EdgeBarrierSubscriber
{
public:
  TestBarrierSubscriber(bool handles = false)
    : handles_(handles)
  {}

  bool HandleBarrierEvent(PointerBarrierWrapper* owner, BarrierEvent::Ptr event)
  {
    return handles_;
  }

  bool handles_;
};

class TestEdgeBarrierController : public Test
{
public:
  virtual void SetUp()
  {
    bc.options = std::make_shared<launcher::Options>();
    bc.options()->edge_passed_disabled_ms = 150;

    uscreen.SetupFakeMultiMonitor();

    for (int i = 0; i < max_num_monitors; ++i)
    {
      // By default we assume that no subscriber handles the events!!!
      bc.Subscribe(&subscribers_[i], i);
    }
  }

  BarrierEvent::Ptr MakeBarrierEvent(int id, bool breaker)
  {
    int velocity = breaker ? std::numeric_limits<int>::max() : bc.options()->edge_overcome_pressure() - 1;
    return std::make_shared<BarrierEvent>(0, 1, velocity, id);
  }

  TestBarrierSubscriber subscribers_[max_num_monitors];
  MockUScreen uscreen;
  MockEdgeBarrierController bc;
};

TEST_F(TestEdgeBarrierController, Construction)
{
  EXPECT_FALSE(bc.sticky_edges);
}

TEST_F(TestEdgeBarrierController, ProcessHandledEvent)
{
  int monitor = 0;

  TestBarrierSubscriber handling_subscriber(true);
  bc.Subscribe(&handling_subscriber, monitor);

  MockPointerBarrier owner(monitor);
  auto breaking_barrier_event = MakeBarrierEvent(0, true);

  EXPECT_CALL(owner, ReleaseBarrier(_)).Times(0);
  bc.ProcessBarrierEvent(&owner, breaking_barrier_event);
}

TEST_F(TestEdgeBarrierController, ProcessHandledEventOnReleasedBarrier)
{
  int monitor = max_num_monitors-1;

  TestBarrierSubscriber handling_subscriber(true);
  bc.Subscribe(&handling_subscriber, monitor);

  MockPointerBarrier owner(monitor, true);
  auto breaking_barrier_event = MakeBarrierEvent(0, true);

  EXPECT_CALL(owner, ReleaseBarrier(_)).Times(0);
  bc.ProcessBarrierEvent(&owner, breaking_barrier_event);
}

TEST_F(TestEdgeBarrierController, ProcessUnHandledEventBreakingBarrier)
{
  int monitor = 1;

  MockPointerBarrier owner(monitor);
  int breaking_id = 12345;
  auto breaking_barrier_event = MakeBarrierEvent(breaking_id, true);

  EXPECT_CALL(owner, ReleaseBarrier(breaking_id));
  bc.ProcessBarrierEvent(&owner, breaking_barrier_event);
}

TEST_F(TestEdgeBarrierController, ProcessUnHandledEventNotBreakingBarrier)
{
  int monitor = 2;

  MockPointerBarrier owner(monitor);
  int not_breaking_id = 54321;
  auto not_breaking_barrier_event = MakeBarrierEvent(not_breaking_id, false);

  EXPECT_CALL(owner, ReleaseBarrier(not_breaking_id)).Times(0);
  bc.ProcessBarrierEvent(&owner, not_breaking_barrier_event);
}

TEST_F(TestEdgeBarrierController, ProcessUnHandledEventOnReleasedBarrier)
{
  int monitor = 2;

  MockPointerBarrier owner(monitor, true);
  int not_breaking_id = 345678;
  auto not_breaking_barrier_event = MakeBarrierEvent(not_breaking_id, false);

  EXPECT_CALL(owner, ReleaseBarrier(not_breaking_id));
  bc.ProcessBarrierEvent(&owner, not_breaking_barrier_event);
}

TEST_F(TestEdgeBarrierController, BreakingEdgeTemporaryReleasesBarrier)
{
  MockPointerBarrier owner;

  EXPECT_CALL(owner, ReleaseBarrier(1));
  bc.ProcessBarrierEvent(&owner, MakeBarrierEvent(1, true));
  ASSERT_TRUE(owner.released());

  Utils::WaitForTimeoutMSec(bc.options()->edge_passed_disabled_ms);
  EXPECT_FALSE(owner.released());
}

TEST_F(TestEdgeBarrierController, BreakingEdgeTemporaryReleasesBarrierForNotHandledEvents)
{
  MockPointerBarrier owner;
  int monitor = 0;
  subscribers_[monitor].handles_ = false;

  EXPECT_CALL(owner, ReleaseBarrier(5));
  bc.ProcessBarrierEvent(&owner, MakeBarrierEvent(5, true));
  ASSERT_TRUE(owner.released());

  subscribers_[monitor].handles_ = false;
  EXPECT_CALL(owner, ReleaseBarrier(6));
  bc.ProcessBarrierEvent(&owner, MakeBarrierEvent(6, false));
}

TEST_F(TestEdgeBarrierController, BreakingEdgeDontReleasesBarrierForHandledEvents)
{
  MockPointerBarrier owner;
  int monitor = 0;
  subscribers_[monitor].handles_ = false;

  EXPECT_CALL(owner, ReleaseBarrier(5));
  bc.ProcessBarrierEvent(&owner, MakeBarrierEvent(5, true));
  ASSERT_TRUE(owner.released());

  subscribers_[monitor].handles_ = true;
  EXPECT_CALL(owner, ReleaseBarrier(_)).Times(0);
  bc.ProcessBarrierEvent(&owner, MakeBarrierEvent(6, true));
}

}
