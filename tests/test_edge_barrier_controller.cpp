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
#include "EdgeBarrierControllerPrivate.h"

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

class TestBarrierSubscriber : public EdgeBarrierSubscriber
{
public:
  TestBarrierSubscriber(EdgeBarrierSubscriber::Result result = EdgeBarrierSubscriber::Result::IGNORED)
    : handle_result_(result)
  {}

  EdgeBarrierSubscriber::Result HandleBarrierEvent(PointerBarrierWrapper* owner, BarrierEvent::Ptr event)
  {
    return handle_result_;
  }

  EdgeBarrierSubscriber::Result handle_result_;
};

} // namespace

namespace unity {
namespace ui {
class TestEdgeBarrierController : public Test
{
public:
  virtual void SetUp()
  {
    bc.options = std::make_shared<launcher::Options>();
    bc.options()->edge_resist = true;
    bc.options()->edge_passed_disabled_ms = 150;

    uscreen.SetupFakeMultiMonitor();

    for (unsigned i = 0; i < monitors::MAX; ++i)
    {
      // By default we assume that no subscriber handles the events!!!
      bc.Subscribe(&subscribers_[i], i);
    }
  }

  void ProcessBarrierEvent(PointerBarrierWrapper* owner, BarrierEvent::Ptr event)
  {
    GetPrivateImpl()->OnPointerBarrierEvent(owner, event);
  }

  EdgeBarrierController::Impl* GetPrivateImpl() const
  {
    return bc.pimpl.get();
  }

  BarrierEvent::Ptr MakeBarrierEvent(int id, bool breaker)
  {
    int velocity = breaker ? std::numeric_limits<int>::max() : bc.options()->edge_overcome_pressure() - 1;
    return std::make_shared<BarrierEvent>(0, 1, velocity, id);
  }


  TestBarrierSubscriber subscribers_[monitors::MAX];
  MockUScreen uscreen;
  EdgeBarrierController bc;
};
} // namespace ui
} // namespace unity

namespace
{

TEST_F(TestEdgeBarrierController, Construction)
{
  EXPECT_TRUE(bc.sticky_edges);

  for (unsigned i = 0; i < monitors::MAX; ++i)
    ASSERT_EQ(bc.GetSubscriber(i), &subscribers_[i]);
}

TEST_F(TestEdgeBarrierController, Unsubscribe)
{
  for (unsigned i = 0; i < monitors::MAX; ++i)
  {
    bc.Unsubscribe(&subscribers_[i], i);
    ASSERT_EQ(bc.GetSubscriber(i), nullptr);
  }
}

TEST_F(TestEdgeBarrierController, UnsubscribeInvalid)
{
  bc.Unsubscribe(&subscribers_[2], 1);
  ASSERT_EQ(bc.GetSubscriber(2), &subscribers_[2]);
}

TEST_F(TestEdgeBarrierController, SubscriberReplace)
{
  TestBarrierSubscriber handling_subscriber(EdgeBarrierSubscriber::Result::HANDLED);
  bc.Subscribe(&handling_subscriber, 0);
  EXPECT_EQ(bc.GetSubscriber(0), &handling_subscriber);
}

TEST_F(TestEdgeBarrierController, ProcessHandledEvent)
{
  int monitor = 0;

  TestBarrierSubscriber handling_subscriber(EdgeBarrierSubscriber::Result::HANDLED);
  bc.Subscribe(&handling_subscriber, monitor);

  MockPointerBarrier owner(monitor);
  auto breaking_barrier_event = MakeBarrierEvent(0, true);

  EXPECT_CALL(owner, ReleaseBarrier(_)).Times(0);
  ProcessBarrierEvent(&owner, breaking_barrier_event);
  EXPECT_EQ(GetPrivateImpl()->decaymulator_.value(), 0);
}

TEST_F(TestEdgeBarrierController, ProcessHandledEventOnReleasedBarrier)
{
  int monitor = monitors::MAX-1;

  TestBarrierSubscriber handling_subscriber(EdgeBarrierSubscriber::Result::HANDLED);
  bc.Subscribe(&handling_subscriber, monitor);

  MockPointerBarrier owner(monitor, true);
  auto breaking_barrier_event = MakeBarrierEvent(5, true);

  EXPECT_CALL(owner, ReleaseBarrier(5)).Times(1);
  ProcessBarrierEvent(&owner, breaking_barrier_event);
}

TEST_F(TestEdgeBarrierController, ProcessUnHandledEventBreakingBarrier)
{
  int monitor = 1;

  MockPointerBarrier owner(monitor);
  int breaking_id = 12345;
  auto breaking_barrier_event = MakeBarrierEvent(breaking_id, true);

  EXPECT_CALL(owner, ReleaseBarrier(breaking_id));
  ProcessBarrierEvent(&owner, breaking_barrier_event);
}

TEST_F(TestEdgeBarrierController, ProcessUnHandledEventBreakingBarrierOnMaxMonitor)
{
  int monitor = monitors::MAX;

  MockPointerBarrier owner(monitor);
  auto breaking_barrier_event = MakeBarrierEvent(0, true);

  EXPECT_CALL(owner, ReleaseBarrier(_));
  ProcessBarrierEvent(&owner, breaking_barrier_event);
}

TEST_F(TestEdgeBarrierController, ProcessUnHandledEventNotBreakingBarrier)
{
  int monitor = 2;

  MockPointerBarrier owner(monitor);
  int not_breaking_id = 54321;
  auto not_breaking_barrier_event = MakeBarrierEvent(not_breaking_id, false);

  EXPECT_CALL(owner, ReleaseBarrier(not_breaking_id)).Times(0);
  ProcessBarrierEvent(&owner, not_breaking_barrier_event);
}

TEST_F(TestEdgeBarrierController, ProcessUnHandledEventOnReleasedBarrier)
{
  int monitor = 2;

  MockPointerBarrier owner(monitor, true);
  int not_breaking_id = 345678;
  auto not_breaking_barrier_event = MakeBarrierEvent(not_breaking_id, false);

  EXPECT_CALL(owner, ReleaseBarrier(not_breaking_id));
  ProcessBarrierEvent(&owner, not_breaking_barrier_event);
}

TEST_F(TestEdgeBarrierController, ProcessAlreadyHandledEvent)
{
  int monitor = g_random_int_range(0, monitors::MAX);

  MockPointerBarrier owner(monitor);
  subscribers_[monitor].handle_result_ = EdgeBarrierSubscriber::Result::ALREADY_HANDLED;

  auto event = MakeBarrierEvent(g_random_int(), false);

  EXPECT_CALL(owner, ReleaseBarrier(_)).Times(0);
  ProcessBarrierEvent(&owner, event);
  EXPECT_EQ(GetPrivateImpl()->decaymulator_.value(), event->velocity);
}

TEST_F(TestEdgeBarrierController, ProcessIgnoredEventWithStickyEdges)
{
  int monitor = g_random_int_range(0, monitors::MAX);

  bc.sticky_edges = true;
  MockPointerBarrier owner(monitor);
  subscribers_[monitor].handle_result_ = EdgeBarrierSubscriber::Result::IGNORED;

  auto event = MakeBarrierEvent(g_random_int(), false);

  EXPECT_CALL(owner, ReleaseBarrier(_)).Times(0);
  ProcessBarrierEvent(&owner, event);
  EXPECT_FALSE(owner.released());
  EXPECT_FALSE(owner.release_once());
  EXPECT_EQ(GetPrivateImpl()->decaymulator_.value(), event->velocity);
}

TEST_F(TestEdgeBarrierController, ProcessIgnoredEventWithOutStickyEdges)
{
  int monitor = g_random_int_range(0, monitors::MAX);

  bc.sticky_edges = false;
  MockPointerBarrier owner(monitor);
  subscribers_[monitor].handle_result_ = EdgeBarrierSubscriber::Result::IGNORED;

  auto event = MakeBarrierEvent(g_random_int(), false);

  EXPECT_CALL(owner, ReleaseBarrier(event->event_id));
  ProcessBarrierEvent(&owner, event);
  EXPECT_TRUE(owner.released());
  EXPECT_TRUE(owner.release_once());
  EXPECT_EQ(GetPrivateImpl()->decaymulator_.value(), 0);
}

TEST_F(TestEdgeBarrierController, ProcessNeedsReleaseEvent)
{
  int monitor = g_random_int_range(0, monitors::MAX);

  MockPointerBarrier owner(monitor);
  subscribers_[monitor].handle_result_ = EdgeBarrierSubscriber::Result::NEEDS_RELEASE;

  auto event = MakeBarrierEvent(g_random_int(), false);

  EXPECT_CALL(owner, ReleaseBarrier(event->event_id));
  ProcessBarrierEvent(&owner, event);
  EXPECT_EQ(GetPrivateImpl()->decaymulator_.value(), 0);
}

TEST_F(TestEdgeBarrierController, BreakingEdgeTemporaryReleasesBarrier)
{
  MockPointerBarrier owner;

  EXPECT_CALL(owner, ReleaseBarrier(1));
  ProcessBarrierEvent(&owner, MakeBarrierEvent(1, true));
  ASSERT_TRUE(owner.released());

  Utils::WaitForTimeoutMSec(bc.options()->edge_passed_disabled_ms);
  EXPECT_FALSE(owner.released());
}

TEST_F(TestEdgeBarrierController, BreakingEdgeTemporaryReleasesBarrierForNotHandledEvents)
{
  MockPointerBarrier owner;
  int monitor = 0;
  subscribers_[monitor].handle_result_ = EdgeBarrierSubscriber::Result::IGNORED;

  EXPECT_CALL(owner, ReleaseBarrier(5));
  ProcessBarrierEvent(&owner, MakeBarrierEvent(5, true));
  ASSERT_TRUE(owner.released());

  subscribers_[monitor].handle_result_ = EdgeBarrierSubscriber::Result::IGNORED;
  EXPECT_CALL(owner, ReleaseBarrier(6));
  ProcessBarrierEvent(&owner, MakeBarrierEvent(6, false));
}

TEST_F(TestEdgeBarrierController, BreakingEdgeTemporaryReleasesBarrierForHandledEvents)
{
  MockPointerBarrier owner;
  int monitor = 0;
  subscribers_[monitor].handle_result_ = EdgeBarrierSubscriber::Result::IGNORED;

  EXPECT_CALL(owner, ReleaseBarrier(5));
  ProcessBarrierEvent(&owner, MakeBarrierEvent(5, true));
  ASSERT_TRUE(owner.released());

  subscribers_[monitor].handle_result_ = EdgeBarrierSubscriber::Result::HANDLED;
  EXPECT_CALL(owner, ReleaseBarrier(6)).Times(1);
  ProcessBarrierEvent(&owner, MakeBarrierEvent(6, true));
}

TEST_F(TestEdgeBarrierController, StickyEdgePropertyProxiesLauncherOption)
{
  bc.options()->edge_resist = false;
  EXPECT_FALSE(bc.sticky_edges());

  bc.options()->edge_resist = true;
  EXPECT_TRUE(bc.sticky_edges());

  bc.sticky_edges = false;
  EXPECT_FALSE(bc.options()->edge_resist());

  bc.sticky_edges = true;
  EXPECT_TRUE(bc.options()->edge_resist());
}

TEST_F(TestEdgeBarrierController, TestTheDirectionIsAlawysSetToBothSides)
{
  for (auto barrier : GetPrivateImpl()->barriers_)
    ASSERT_EQ(barrier->direction, BarrierDirection::BOTH);
}

TEST_F(TestEdgeBarrierController, BarrierDoesNotBreakIfYEventToFarApart)
{
  MockPointerBarrier owner;

  int velocity = bc.options()->edge_overcome_pressure() * bc.options()->edge_responsiveness();
  auto firstEvent = std::make_shared<BarrierEvent>(0, 50, velocity, 10);
  auto secondEvent = std::make_shared<BarrierEvent>(0, 150, velocity, 11);

  EXPECT_CALL(owner, ReleaseBarrier(_)).Times(0);
  ProcessBarrierEvent(&owner, firstEvent);
  ProcessBarrierEvent(&owner, secondEvent);
}

TEST_F(TestEdgeBarrierController, BarrierBreaksInYBreakZone)
{
  MockPointerBarrier owner;

  int velocity = bc.options()->edge_overcome_pressure() * bc.options()->edge_responsiveness();
  auto firstEvent = std::make_shared<BarrierEvent>(0, 50, velocity, 10);

  EXPECT_CALL(owner, ReleaseBarrier(_)).Times(1);
  ProcessBarrierEvent(&owner, firstEvent);
  ProcessBarrierEvent(&owner, firstEvent);
}

TEST_F(TestEdgeBarrierController, BarrierReleaseIfNoSubscriberForMonitor)
{
  MockPointerBarrier owner(monitors::MAX);
  auto firstEvent = std::make_shared<BarrierEvent>(0, 50, 1, 10);

  EXPECT_CALL(owner, ReleaseBarrier(_)).Times(1);
  ProcessBarrierEvent(&owner, firstEvent);
}

}
