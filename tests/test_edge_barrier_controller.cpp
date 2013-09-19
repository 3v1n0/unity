/*
 * Copyright 2012-2013 Canonical Ltd.
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
 *              Andrea Azzarone <andrea.azzarone@canonical.com>
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

class MockPointerBarrierWrapper : public PointerBarrierWrapper
{
public:
  MockPointerBarrierWrapper(int monitor = 0, bool released_ = false, BarrierOrientation orientation_ = VERTICAL)
  {
    index = monitor;
    x1 = 1;
    released = released_;
    orientation = orientation_;
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
      bc.AddVerticalSubscriber(&vertical_subscribers_[i], i);
      bc.AddHorizontalSubscriber(&horizontal_subscribers_[i], i);
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


  TestBarrierSubscriber horizontal_subscribers_[monitors::MAX];
  TestBarrierSubscriber vertical_subscribers_[monitors::MAX];
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
  {
    ASSERT_EQ(bc.GetVerticalSubscriber(i), &vertical_subscribers_[i]);
    ASSERT_EQ(bc.GetHorizontalSubscriber(i), &horizontal_subscribers_[i]);
  }
}

TEST_F(TestEdgeBarrierController, UscreenChangedSignalDisconnection)
{
  {
    EdgeBarrierController bc;
    bc.options = std::make_shared<launcher::Options>();
  }

  // Make sure it does not crash.
  uscreen.changed.emit(uscreen.GetPrimaryMonitor(), uscreen.GetMonitors());
}

TEST_F(TestEdgeBarrierController, LauncherOptionChangedSignalDisconnection)
{
  auto options = std::make_shared<launcher::Options>();

  {
    EdgeBarrierController bc;
    bc.options = options;
  }

  // Make sure it does not crash.
  options->option_changed.emit();
}

TEST_F(TestEdgeBarrierController, RemoveVerticalSubscriber)
{
  for (unsigned i = 0; i < monitors::MAX; ++i)
  {
    bc.RemoveVerticalSubscriber(&vertical_subscribers_[i], i);
    ASSERT_EQ(bc.GetVerticalSubscriber(i), nullptr);
  }
}

TEST_F(TestEdgeBarrierController, RemoveHorizontalSubscriber)
{
  for (unsigned i = 0; i < monitors::MAX; ++i)
  {
    bc.RemoveHorizontalSubscriber(&horizontal_subscribers_[i], i);
    ASSERT_EQ(bc.GetHorizontalSubscriber(i), nullptr);
  }
}

TEST_F(TestEdgeBarrierController, RemoveVerticalSubscriberInvalid)
{
  bc.RemoveVerticalSubscriber(&vertical_subscribers_[2], 1);
  ASSERT_EQ(bc.GetVerticalSubscriber(2), &vertical_subscribers_[2]);
}

TEST_F(TestEdgeBarrierController, RemoveHorizontalSubscriberInvalid)
{
  bc.RemoveHorizontalSubscriber(&horizontal_subscribers_[2], 1);
  ASSERT_EQ(bc.GetHorizontalSubscriber(2), &horizontal_subscribers_[2]);
}

TEST_F(TestEdgeBarrierController, VerticalSubscriberReplace)
{
  TestBarrierSubscriber handling_subscriber(EdgeBarrierSubscriber::Result::HANDLED);
  bc.AddVerticalSubscriber(&handling_subscriber, 0);
  EXPECT_EQ(bc.GetVerticalSubscriber(0), &handling_subscriber);
}

TEST_F(TestEdgeBarrierController, HorizontalSubscriberReplace)
{
  TestBarrierSubscriber handling_subscriber(EdgeBarrierSubscriber::Result::HANDLED);
  bc.AddHorizontalSubscriber(&handling_subscriber, 0);
  EXPECT_EQ(bc.GetHorizontalSubscriber(0), &handling_subscriber);
}

TEST_F(TestEdgeBarrierController, ProcessHandledEventForVerticalSubscriber)
{
  int monitor = 0;

  TestBarrierSubscriber handling_subscriber(EdgeBarrierSubscriber::Result::HANDLED);
  bc.AddVerticalSubscriber(&handling_subscriber, monitor);

  MockPointerBarrierWrapper owner(monitor, false, VERTICAL);
  auto breaking_barrier_event = MakeBarrierEvent(0, true);

  EXPECT_CALL(owner, ReleaseBarrier(_)).Times(0);
  ProcessBarrierEvent(&owner, breaking_barrier_event);
  EXPECT_EQ(GetPrivateImpl()->decaymulator_.value(), 0);
}

TEST_F(TestEdgeBarrierController, ProcessHandledEventForHorizontalSubscriber)
{
  int monitor = 0;

  TestBarrierSubscriber handling_subscriber(EdgeBarrierSubscriber::Result::HANDLED);
  bc.AddHorizontalSubscriber(&handling_subscriber, monitor);

  MockPointerBarrierWrapper owner(monitor, false, HORIZONTAL);
  auto breaking_barrier_event = MakeBarrierEvent(0, true);

  EXPECT_CALL(owner, ReleaseBarrier(_)).Times(0);
  ProcessBarrierEvent(&owner, breaking_barrier_event);
  EXPECT_EQ(GetPrivateImpl()->decaymulator_.value(), 0);
}

TEST_F(TestEdgeBarrierController, ProcessHandledEventOnReleasedBarrierForVerticalSubscriber)
{
  int monitor = monitors::MAX-1;

  TestBarrierSubscriber handling_subscriber(EdgeBarrierSubscriber::Result::HANDLED);
  bc.AddVerticalSubscriber(&handling_subscriber, monitor);

  MockPointerBarrierWrapper owner(monitor, true, VERTICAL);
  auto breaking_barrier_event = MakeBarrierEvent(5, true);

  EXPECT_CALL(owner, ReleaseBarrier(5)).Times(1);
  ProcessBarrierEvent(&owner, breaking_barrier_event);
}

TEST_F(TestEdgeBarrierController, ProcessHandledEventOnReleasedBarrierForHorizontalSubscriber)
{
  int monitor = monitors::MAX-1;

  TestBarrierSubscriber handling_subscriber(EdgeBarrierSubscriber::Result::HANDLED);
  bc.AddHorizontalSubscriber(&handling_subscriber, monitor);

  MockPointerBarrierWrapper owner(monitor, true, HORIZONTAL);
  auto breaking_barrier_event = MakeBarrierEvent(5, true);

  EXPECT_CALL(owner, ReleaseBarrier(5)).Times(1);
  ProcessBarrierEvent(&owner, breaking_barrier_event);
}

TEST_F(TestEdgeBarrierController, ProcessUnHandledEventBreakingBarrierForVerticalSubscriber)
{
  int monitor = 1;

  MockPointerBarrierWrapper owner(monitor, false, VERTICAL);
  int breaking_id = 12345;
  auto breaking_barrier_event = MakeBarrierEvent(breaking_id, true);

  EXPECT_CALL(owner, ReleaseBarrier(breaking_id));
  ProcessBarrierEvent(&owner, breaking_barrier_event);
}

TEST_F(TestEdgeBarrierController, ProcessUnHandledEventBreakingBarrierForHorizontalSubscriber)
{
  int monitor = 1;

  MockPointerBarrierWrapper owner(monitor, false, HORIZONTAL);
  int breaking_id = 12345;
  auto breaking_barrier_event = MakeBarrierEvent(breaking_id, true);

  EXPECT_CALL(owner, ReleaseBarrier(breaking_id));
  ProcessBarrierEvent(&owner, breaking_barrier_event);
}

TEST_F(TestEdgeBarrierController, ProcessUnHandledEventBreakingBarrierOnMaxMonitorForVerticalSubscriber)
{
  int monitor = monitors::MAX;

  MockPointerBarrierWrapper owner(monitor, false, VERTICAL);
  auto breaking_barrier_event = MakeBarrierEvent(0, true);

  EXPECT_CALL(owner, ReleaseBarrier(_));
  ProcessBarrierEvent(&owner, breaking_barrier_event);
}

TEST_F(TestEdgeBarrierController, ProcessUnHandledEventBreakingBarrierOnMaxMonitorForHorizontalSubscriber)
{
  int monitor = monitors::MAX;

  MockPointerBarrierWrapper owner(monitor, false, HORIZONTAL);
  auto breaking_barrier_event = MakeBarrierEvent(0, true);

  EXPECT_CALL(owner, ReleaseBarrier(_));
  ProcessBarrierEvent(&owner, breaking_barrier_event);
}

TEST_F(TestEdgeBarrierController, ProcessUnHandledEventNotBreakingBarrierForVerticalSubscriber)
{
  int monitor = 2;

  MockPointerBarrierWrapper owner(monitor, false, VERTICAL);
  int not_breaking_id = 54321;
  auto not_breaking_barrier_event = MakeBarrierEvent(not_breaking_id, false);

  EXPECT_CALL(owner, ReleaseBarrier(not_breaking_id)).Times(0);
  ProcessBarrierEvent(&owner, not_breaking_barrier_event);
}

TEST_F(TestEdgeBarrierController, ProcessUnHandledEventNotBreakingBarrierForHorizontalSubscriber)
{
  int monitor = 2;

  MockPointerBarrierWrapper owner(monitor, false, HORIZONTAL);
  int not_breaking_id = 54321;
  auto not_breaking_barrier_event = MakeBarrierEvent(not_breaking_id, false);

  EXPECT_CALL(owner, ReleaseBarrier(not_breaking_id)).Times(0);
  ProcessBarrierEvent(&owner, not_breaking_barrier_event);
}

TEST_F(TestEdgeBarrierController, ProcessUnHandledEventOnReleasedBarrierForVerticalSubscriber)
{
  int monitor = 2;

  MockPointerBarrierWrapper owner(monitor, true, VERTICAL);
  int not_breaking_id = 345678;
  auto not_breaking_barrier_event = MakeBarrierEvent(not_breaking_id, false);

  EXPECT_CALL(owner, ReleaseBarrier(not_breaking_id));
  ProcessBarrierEvent(&owner, not_breaking_barrier_event);
}

TEST_F(TestEdgeBarrierController, ProcessUnHandledEventOnReleasedBarrierForHorizontalSubscriber)
{
  int monitor = 2;

  MockPointerBarrierWrapper owner(monitor, true, HORIZONTAL);
  int not_breaking_id = 345678;
  auto not_breaking_barrier_event = MakeBarrierEvent(not_breaking_id, false);

  EXPECT_CALL(owner, ReleaseBarrier(not_breaking_id));
  ProcessBarrierEvent(&owner, not_breaking_barrier_event);
}

TEST_F(TestEdgeBarrierController, ProcessAlreadyHandledEventForVerticalSubscriber)
{
  int monitor = g_random_int_range(0, monitors::MAX);

  MockPointerBarrierWrapper owner(monitor, false, VERTICAL);
  vertical_subscribers_[monitor].handle_result_ = EdgeBarrierSubscriber::Result::ALREADY_HANDLED;

  auto event = MakeBarrierEvent(g_random_int(), false);

  EXPECT_CALL(owner, ReleaseBarrier(_)).Times(0);
  ProcessBarrierEvent(&owner, event);
  EXPECT_EQ(GetPrivateImpl()->decaymulator_.value(), event->velocity);
}

TEST_F(TestEdgeBarrierController, ProcessAlreadyHandledEventForHorizontalSubscriber)
{
  int monitor = g_random_int_range(0, monitors::MAX);

  MockPointerBarrierWrapper owner(monitor, false, HORIZONTAL);
  horizontal_subscribers_[monitor].handle_result_ = EdgeBarrierSubscriber::Result::ALREADY_HANDLED;

  auto event = MakeBarrierEvent(g_random_int(), false);

  EXPECT_CALL(owner, ReleaseBarrier(_)).Times(0);
  ProcessBarrierEvent(&owner, event);
  EXPECT_EQ(GetPrivateImpl()->decaymulator_.value(), event->velocity);
}

TEST_F(TestEdgeBarrierController, ProcessIgnoredEventWithStickyEdgesForVerticalSubscriber)
{
  int monitor = g_random_int_range(0, monitors::MAX);

  bc.sticky_edges = true;
  MockPointerBarrierWrapper owner(monitor, false, VERTICAL);
  vertical_subscribers_[monitor].handle_result_ = EdgeBarrierSubscriber::Result::IGNORED;

  auto event = MakeBarrierEvent(g_random_int(), false);

  EXPECT_CALL(owner, ReleaseBarrier(_)).Times(0);
  ProcessBarrierEvent(&owner, event);
  EXPECT_FALSE(owner.released());
  EXPECT_FALSE(owner.release_once());
  EXPECT_EQ(GetPrivateImpl()->decaymulator_.value(), event->velocity);
}

TEST_F(TestEdgeBarrierController, ProcessIgnoredEventWithStickyEdgesForHorizontalSubscriber)
{
  int monitor = g_random_int_range(0, monitors::MAX);

  bc.sticky_edges = true;
  MockPointerBarrierWrapper owner(monitor, false, HORIZONTAL);
  horizontal_subscribers_[monitor].handle_result_ = EdgeBarrierSubscriber::Result::IGNORED;

  auto event = MakeBarrierEvent(g_random_int(), false);

  EXPECT_CALL(owner, ReleaseBarrier(_)).Times(0);
  ProcessBarrierEvent(&owner, event);
  EXPECT_FALSE(owner.released());
  EXPECT_FALSE(owner.release_once());
  EXPECT_EQ(GetPrivateImpl()->decaymulator_.value(), event->velocity);
}

TEST_F(TestEdgeBarrierController, ProcessIgnoredEventWithOutStickyEdgesForVerticalSubscriber)
{
  int monitor = g_random_int_range(0, monitors::MAX);

  bc.sticky_edges = false;
  MockPointerBarrierWrapper owner(monitor, false, VERTICAL);
  vertical_subscribers_[monitor].handle_result_ = EdgeBarrierSubscriber::Result::IGNORED;

  auto event = MakeBarrierEvent(g_random_int(), false);

  EXPECT_CALL(owner, ReleaseBarrier(event->event_id));
  ProcessBarrierEvent(&owner, event);
  EXPECT_TRUE(owner.released());
  EXPECT_TRUE(owner.release_once());
  EXPECT_EQ(GetPrivateImpl()->decaymulator_.value(), 0);
}

TEST_F(TestEdgeBarrierController, ProcessIgnoredEventWithOutStickyEdgesForHoriziontalSubscriber)
{
  int monitor = g_random_int_range(0, monitors::MAX);

  bc.sticky_edges = false;
  MockPointerBarrierWrapper owner(monitor, false, HORIZONTAL);
  horizontal_subscribers_[monitor].handle_result_ = EdgeBarrierSubscriber::Result::IGNORED;

  auto event = MakeBarrierEvent(g_random_int(), false);

  EXPECT_CALL(owner, ReleaseBarrier(event->event_id));
  ProcessBarrierEvent(&owner, event);
  EXPECT_TRUE(owner.released());
  EXPECT_TRUE(owner.release_once());
  EXPECT_EQ(GetPrivateImpl()->decaymulator_.value(), 0);
}

TEST_F(TestEdgeBarrierController, ProcessNeedsReleaseEventForVerticalSubscriber)
{
  int monitor = g_random_int_range(0, monitors::MAX);

  MockPointerBarrierWrapper owner(monitor, false, VERTICAL);
  vertical_subscribers_[monitor].handle_result_ = EdgeBarrierSubscriber::Result::NEEDS_RELEASE;

  auto event = MakeBarrierEvent(g_random_int(), false);

  EXPECT_CALL(owner, ReleaseBarrier(event->event_id));
  ProcessBarrierEvent(&owner, event);
  EXPECT_EQ(GetPrivateImpl()->decaymulator_.value(), 0);
}

TEST_F(TestEdgeBarrierController, ProcessNeedsReleaseEventForHorizontalSubscriber)
{
  int monitor = g_random_int_range(0, monitors::MAX);

  MockPointerBarrierWrapper owner(monitor, false, HORIZONTAL);
  horizontal_subscribers_[monitor].handle_result_ = EdgeBarrierSubscriber::Result::NEEDS_RELEASE;

  auto event = MakeBarrierEvent(g_random_int(), false);

  EXPECT_CALL(owner, ReleaseBarrier(event->event_id));
  ProcessBarrierEvent(&owner, event);
  EXPECT_EQ(GetPrivateImpl()->decaymulator_.value(), 0);
}

TEST_F(TestEdgeBarrierController, BreakingEdgeTemporaryReleasesBarrierForVerticalSubscriber)
{
  int monitor = g_random_int_range(0, monitors::MAX);
  MockPointerBarrierWrapper owner(monitor, false, VERTICAL);

  EXPECT_CALL(owner, ReleaseBarrier(1));
  ProcessBarrierEvent(&owner, MakeBarrierEvent(1, true));
  ASSERT_TRUE(owner.released());

  Utils::WaitForTimeoutMSec(bc.options()->edge_passed_disabled_ms);
  EXPECT_FALSE(owner.released());
}

TEST_F(TestEdgeBarrierController, BreakingEdgeTemporaryReleasesBarrierForHoriziontalSubscriber)
{
  int monitor = g_random_int_range(0, monitors::MAX);
  MockPointerBarrierWrapper owner(monitor, false, HORIZONTAL);

  EXPECT_CALL(owner, ReleaseBarrier(1));
  ProcessBarrierEvent(&owner, MakeBarrierEvent(1, true));
  ASSERT_TRUE(owner.released());

  Utils::WaitForTimeoutMSec(bc.options()->edge_passed_disabled_ms);
  EXPECT_FALSE(owner.released());
}

TEST_F(TestEdgeBarrierController, BreakingEdgeTemporaryReleasesBarrierForNotHandledEventsForVerticalSubscriber)
{
  int monitor = 0;
  MockPointerBarrierWrapper owner(monitor, false, VERTICAL);
  vertical_subscribers_[monitor].handle_result_ = EdgeBarrierSubscriber::Result::IGNORED;

  EXPECT_CALL(owner, ReleaseBarrier(5));
  ProcessBarrierEvent(&owner, MakeBarrierEvent(5, true));
  ASSERT_TRUE(owner.released());

  vertical_subscribers_[monitor].handle_result_ = EdgeBarrierSubscriber::Result::IGNORED;
  EXPECT_CALL(owner, ReleaseBarrier(6));
  ProcessBarrierEvent(&owner, MakeBarrierEvent(6, false));
}

TEST_F(TestEdgeBarrierController, BreakingEdgeTemporaryReleasesBarrierForNotHandledEventsForHorizontalSubscriber)
{
  int monitor = 0;
  MockPointerBarrierWrapper owner(monitor, false, HORIZONTAL);
  horizontal_subscribers_[monitor].handle_result_ = EdgeBarrierSubscriber::Result::IGNORED;

  EXPECT_CALL(owner, ReleaseBarrier(5));
  ProcessBarrierEvent(&owner, MakeBarrierEvent(5, true));
  ASSERT_TRUE(owner.released());

  horizontal_subscribers_[monitor].handle_result_ = EdgeBarrierSubscriber::Result::IGNORED;
  EXPECT_CALL(owner, ReleaseBarrier(6));
  ProcessBarrierEvent(&owner, MakeBarrierEvent(6, false));
}

TEST_F(TestEdgeBarrierController, BreakingEdgeTemporaryReleasesBarrierForHandledEventsForVerticalSubscriber)
{
  int monitor = 0;
  MockPointerBarrierWrapper owner(monitor, false, VERTICAL);
  vertical_subscribers_[monitor].handle_result_ = EdgeBarrierSubscriber::Result::IGNORED;

  EXPECT_CALL(owner, ReleaseBarrier(5));
  ProcessBarrierEvent(&owner, MakeBarrierEvent(5, true));
  ASSERT_TRUE(owner.released());

  vertical_subscribers_[monitor].handle_result_ = EdgeBarrierSubscriber::Result::HANDLED;
  EXPECT_CALL(owner, ReleaseBarrier(6)).Times(1);
  ProcessBarrierEvent(&owner, MakeBarrierEvent(6, true));
}

TEST_F(TestEdgeBarrierController, BreakingEdgeTemporaryReleasesBarrierForHandledEventsForHoriziontalSubscriber)
{
  int monitor = 0;
  MockPointerBarrierWrapper owner(monitor, false, HORIZONTAL);
  horizontal_subscribers_[monitor].handle_result_ = EdgeBarrierSubscriber::Result::IGNORED;

  EXPECT_CALL(owner, ReleaseBarrier(5));
  ProcessBarrierEvent(&owner, MakeBarrierEvent(5, true));
  ASSERT_TRUE(owner.released());

  horizontal_subscribers_[monitor].handle_result_ = EdgeBarrierSubscriber::Result::HANDLED;
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
  for (auto barrier : GetPrivateImpl()->vertical_barriers_)
    ASSERT_EQ(barrier->direction, BarrierDirection::BOTH);
}

TEST_F(TestEdgeBarrierController, TestTheDirectionIsAlawysSetToUp)
{
  for (auto barrier : GetPrivateImpl()->horizontal_barriers_)
    ASSERT_EQ(barrier->direction, BarrierDirection::UP);
}

TEST_F(TestEdgeBarrierController, VerticalBarrierDoesNotBreakIfYEventToFarApart)
{
  MockPointerBarrierWrapper owner(0, false, VERTICAL);

  int velocity = bc.options()->edge_overcome_pressure() * bc.options()->edge_responsiveness();
  auto firstEvent = std::make_shared<BarrierEvent>(0, 50, velocity, 10);
  auto secondEvent = std::make_shared<BarrierEvent>(0, 150, velocity, 11);

  EXPECT_CALL(owner, ReleaseBarrier(_)).Times(0);
  ProcessBarrierEvent(&owner, firstEvent);
  ProcessBarrierEvent(&owner, secondEvent);
}

TEST_F(TestEdgeBarrierController, HorizontalBarrierDoesNotBreakIfXEventToFarApart)
{
  MockPointerBarrierWrapper owner(0, false, HORIZONTAL);

  int velocity = bc.options()->edge_overcome_pressure() * bc.options()->edge_responsiveness();
  auto firstEvent = std::make_shared<BarrierEvent>(50, 0, velocity, 10);
  auto secondEvent = std::make_shared<BarrierEvent>(150, 0, velocity, 11);

  EXPECT_CALL(owner, ReleaseBarrier(_)).Times(0);
  ProcessBarrierEvent(&owner, firstEvent);
  ProcessBarrierEvent(&owner, secondEvent);
}

TEST_F(TestEdgeBarrierController, VerticalBarrierBreaksInYBreakZone)
{
  MockPointerBarrierWrapper owner(0, false, VERTICAL);

  int velocity = bc.options()->edge_overcome_pressure() * bc.options()->edge_responsiveness();
  auto firstEvent = std::make_shared<BarrierEvent>(0, 50, velocity, 10);

  EXPECT_CALL(owner, ReleaseBarrier(_)).Times(1);
  ProcessBarrierEvent(&owner, firstEvent);
  ProcessBarrierEvent(&owner, firstEvent);
}

TEST_F(TestEdgeBarrierController, HorizontalBarrierBreaksInXBreakZone)
{
  MockPointerBarrierWrapper owner(0, false, HORIZONTAL);

  int velocity = bc.options()->edge_overcome_pressure() * bc.options()->edge_responsiveness();
  auto firstEvent = std::make_shared<BarrierEvent>(50, 0, velocity, 10);

  EXPECT_CALL(owner, ReleaseBarrier(_)).Times(1);
  ProcessBarrierEvent(&owner, firstEvent);
  ProcessBarrierEvent(&owner, firstEvent);
}

TEST_F(TestEdgeBarrierController, VerticalBarrierReleaseIfNoSubscriberForMonitor)
{
  MockPointerBarrierWrapper owner(monitors::MAX, false, VERTICAL);
  auto firstEvent = std::make_shared<BarrierEvent>(0, 50, 1, 10);

  EXPECT_CALL(owner, ReleaseBarrier(_)).Times(1);
  ProcessBarrierEvent(&owner, firstEvent);
}

TEST_F(TestEdgeBarrierController, HorizontalBarrierReleaseIfNoSubscriberForMonitor)
{
  MockPointerBarrierWrapper owner(monitors::MAX, false, HORIZONTAL);
  auto firstEvent = std::make_shared<BarrierEvent>(50, 0, 1, 10);

  EXPECT_CALL(owner, ReleaseBarrier(_)).Times(1);
  ProcessBarrierEvent(&owner, firstEvent);
}

}
