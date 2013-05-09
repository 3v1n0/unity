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
 * Authored by: Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 *
 */

#include <gtest/gtest.h>
#include <time.h>
#include "TimeUtil.h"
#include "test_utils.h"

#include <UnityCore/GLibSource.h>


using namespace unity::glib;

namespace
{

bool callback_called = false;
unsigned int callback_call_count = 0;

bool OnSourceCallbackStop()
{
  callback_called = true;
  ++callback_call_count;

  return false;
}

bool OnSourceCallbackContinue()
{
  callback_called = true;
  ++callback_call_count;

  return true;
}


// GLib Source tests

TEST(TestGLibSource, ID)
{
  Idle source;
  EXPECT_EQ(source.Id(), 0);
}

TEST(TestGLibSource, Running)
{
  Idle source;
  EXPECT_FALSE(source.IsRunning());
}

TEST(TestGLibSource, Priority)
{
  Idle source;

  source.SetPriority(Source::Priority::HIGH);
  EXPECT_EQ(source.GetPriority(), Source::Priority::HIGH);

  source.SetPriority(Source::Priority::DEFAULT);
  EXPECT_EQ(source.GetPriority(), Source::Priority::DEFAULT);

  source.SetPriority(Source::Priority::HIGH_IDLE);
  EXPECT_EQ(source.GetPriority(), Source::Priority::HIGH_IDLE);

  source.SetPriority(Source::Priority::DEFAULT_IDLE);
  EXPECT_EQ(source.GetPriority(), Source::Priority::DEFAULT_IDLE);

  source.SetPriority(Source::Priority::LOW);
  EXPECT_EQ(source.GetPriority(), Source::Priority::LOW);
}


// GLib Timeout tests

TEST(TestGLibTimeout, Construction)
{
  Timeout timeout(1000, &OnSourceCallbackContinue);
  EXPECT_NE(timeout.Id(), 0);
  EXPECT_TRUE(timeout.IsRunning());
  EXPECT_EQ(timeout.GetPriority(), Source::Priority::DEFAULT);
}

TEST(TestGLibTimeout, ConstructionEmptyCallback)
{
  Timeout timeout(1000, Source::Callback());
  EXPECT_NE(timeout.Id(), 0);
  EXPECT_TRUE(timeout.IsRunning());
  EXPECT_EQ(timeout.GetPriority(), Source::Priority::DEFAULT);
}

TEST(TestGLibTimeout, DelayedRunConstruction)
{
  Timeout timeout(1000);
  EXPECT_EQ(timeout.Id(), 0);
  EXPECT_FALSE(timeout.IsRunning());
  EXPECT_EQ(timeout.GetPriority(), Source::Priority::DEFAULT);
}

TEST(TestGLibTimeout, Destroy)
{
  callback_called = false;
  callback_call_count = 0;
  bool removed_called = false;

  {
    Timeout timeout(1000, &OnSourceCallbackContinue);
    timeout.removed.connect([&] (unsigned int id) { removed_called = true; });
  }

  EXPECT_TRUE(removed_called);
  EXPECT_EQ(callback_call_count, 0);
}

TEST(TestGLibTimeout, OneShotRun)
{
  callback_called = false;
  callback_call_count = 0;
  bool removed_called = false;

  Timeout timeout(100, &OnSourceCallbackStop);
  timeout.removed.connect([&] (unsigned int id) { removed_called = true; });

  Utils::WaitUntilMSec([&timeout] {return timeout.IsRunning();}, false, 500);
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(callback_call_count, 1);
  EXPECT_TRUE(removed_called);
}

TEST(TestGLibTimeout, MultipleShotsRun)
{
  callback_called = false;
  callback_call_count = 0;
  bool removed_called = false;

  {
  auto check_function = []() { return (callback_call_count > 1) ? true : false; };
  Timeout timeout(100, &OnSourceCallbackContinue);
  timeout.removed.connect([&] (unsigned int id) { removed_called = true; });
  Utils::WaitUntil(check_function, true, 1);
  EXPECT_TRUE(timeout.IsRunning());
  }

  EXPECT_TRUE(callback_called);
  EXPECT_GT(callback_call_count, 1);
  EXPECT_TRUE(removed_called);
}

TEST(TestGLibTimeout, OneShotRunWithEmptyCallback)
{
  bool removed_called = false;
  Timeout timeout(100, Source::Callback());
  timeout.removed.connect([&] (unsigned int id) { removed_called = true; });

  Utils::WaitUntilMSec([&timeout] {return timeout.IsRunning();}, false, 500);
  EXPECT_TRUE(removed_called);
}

TEST(TestGLibTimeout, Removal)
{
  callback_called = false;
  callback_call_count = 0;
  bool removed_called = false;

  Timeout timeout(300, &OnSourceCallbackContinue);
  timeout.removed.connect([&] (unsigned int id) { removed_called = true; });

  Utils::WaitForTimeoutMSec(100);
  timeout.Remove();

  Utils::WaitUntilMSec([&timeout] {return timeout.IsRunning();}, false, 300);

  EXPECT_NE(timeout.Id(), 0);
  EXPECT_TRUE(removed_called);
  EXPECT_FALSE(callback_called);
  EXPECT_EQ(callback_call_count, 0);
}

TEST(TestGLibTimeout, Running)
{
  callback_called = false;
  callback_call_count = 0;

  Timeout timeout(300, Source::Priority::HIGH);
  EXPECT_FALSE(timeout.IsRunning());

  timeout.Run(&OnSourceCallbackStop);
  EXPECT_TRUE(timeout.IsRunning());

  Utils::WaitUntilMSec([&timeout] {return timeout.IsRunning();}, false, 500);
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(callback_call_count, 1);
}

TEST(TestGLibTimeout, RemoveOnCallback)
{
  bool local_callback_called = false;
  unsigned int local_callback_call_count = 0;

  Timeout timeout(10, [&] {
    local_callback_called = true;
    ++local_callback_call_count;
    timeout.Remove();

    // this function would be called more than once if we had not removed the source.
    return true;
  });

  Utils::WaitForTimeoutMSec(100);

  ASSERT_EQ(timeout.IsRunning(), false);
  EXPECT_EQ(local_callback_called, true);
  EXPECT_EQ(local_callback_call_count, 1);
}

TEST(TestGLibTimeout, RemovePtrOnCallback)
{
  bool local_callback_called = false;
  unsigned int local_callback_call_count = 0;

  Source::UniquePtr timeout(new Timeout(10, [&] {
    unsigned int id = timeout->Id();
    local_callback_called = true;
    local_callback_call_count++;

    timeout.reset();
    // resetting the ptr should destroy the source.
    EXPECT_TRUE(g_main_context_find_source_by_id(NULL, id) == nullptr);
    // this function would be called more than once (local_callback_call_count > 1) if we had not removed the source.
    return true;
  }));

  Utils::WaitForTimeoutMSec(100);

  ASSERT_EQ(timeout, nullptr);
  EXPECT_EQ(local_callback_called, true);
  EXPECT_EQ(local_callback_call_count, 1);
}

TEST(TestGLibTimeout, AutoRemoveSourceOnCallback)
{
  bool local_callback_called = false;
  unsigned int local_callback_call_count = 0;

  Source::UniquePtr timeout(new Timeout(10, [&] {
    local_callback_called = true;
    ++local_callback_call_count;

    // return false to remove source.
    return false;
  }));
  unsigned int id = timeout->Id();

  Utils::WaitForTimeoutMSec(100);

  timeout.reset();
  EXPECT_EQ(local_callback_called, true);
  EXPECT_EQ(local_callback_call_count, 1);

  // source should be removed by now.
  EXPECT_TRUE(g_main_context_find_source_by_id(NULL, id) == nullptr);
}


// GLib TimeoutSeconds tests

TEST(TestGLibTimeoutSeconds, Construction)
{
  TimeoutSeconds timeout(1, &OnSourceCallbackContinue);
  EXPECT_NE(timeout.Id(), 0);
  EXPECT_TRUE(timeout.IsRunning());
  EXPECT_EQ(timeout.GetPriority(), Source::Priority::DEFAULT);
}

TEST(TestGLibTimeoutSeconds, DelayedRunConstruction)
{
  TimeoutSeconds timeout(1);
  EXPECT_EQ(timeout.Id(), 0);
  EXPECT_FALSE(timeout.IsRunning());
  EXPECT_EQ(timeout.GetPriority(), Source::Priority::DEFAULT);
}

TEST(TestGLibTimeoutSeconds, Destroy)
{
  callback_called = false;
  callback_call_count = 0;
  bool removed_called = false;

  {
    TimeoutSeconds timeout(1, &OnSourceCallbackContinue);
    timeout.removed.connect([&] (unsigned int id) { removed_called = true; });
  }

  EXPECT_TRUE(removed_called);
  EXPECT_EQ(callback_call_count, 0);
}

TEST(TestGLibTimeoutSeconds, OneShotRun)
{
  callback_called = false;
  callback_call_count = 0;
  bool removed_called = false;

  TimeoutSeconds timeout(1, &OnSourceCallbackStop);
  timeout.removed.connect([&] (unsigned int id) { removed_called = true; });

  Utils::WaitUntil([&timeout] {return timeout.IsRunning();}, false, 2);
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(callback_call_count, 1);
  EXPECT_TRUE(removed_called);
}

TEST(TestGLibTimeoutSeconds, MultipleShotsRun)
{
  callback_called = false;
  callback_call_count = 0;
  bool removed_called = false;

  {
  auto check_function = []() { return (callback_call_count > 1) ? true : false; };
  TimeoutSeconds timeout(1, &OnSourceCallbackContinue);
  timeout.removed.connect([&] (unsigned int id) { removed_called = true; });
  Utils::WaitUntil(check_function, true, 4);
  EXPECT_TRUE(timeout.IsRunning());
  }

  EXPECT_TRUE(callback_called);
  EXPECT_GT(callback_call_count, 1);
  EXPECT_TRUE(removed_called);
}

// GLib Idle tests

TEST(TestGLibIdle, Construction)
{
  Idle idle(&OnSourceCallbackStop);
  EXPECT_NE(idle.Id(), 0);
  EXPECT_EQ(idle.GetPriority(), Source::Priority::DEFAULT_IDLE);
}

TEST(TestGLibIdle, Destroy)
{
  callback_called = false;
  callback_call_count = 0;
  bool removed_called = false;

  {
    Idle idle(&OnSourceCallbackContinue);
    idle.removed.connect([&] (unsigned int id) { removed_called = true; });
  }

  EXPECT_TRUE(removed_called);
  EXPECT_EQ(callback_call_count, 0);
}

TEST(TestGLibIdle, OneShotRun)
{
  callback_called = false;
  callback_call_count = 0;
  long long pre = 0;
  long long post = 0;

  pre = g_get_monotonic_time();
  Idle idle(&OnSourceCallbackStop);
  idle.removed.connect([&] (unsigned int id) { post = g_get_monotonic_time(); });

  Utils::WaitUntilMSec([&idle] {return idle.IsRunning();}, false, 100);
  EXPECT_FALSE(idle.IsRunning());
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(callback_call_count, 1);
  EXPECT_LE(pre, post);
}

TEST(TestGLibIdle, MultipleShotsRun)
{
  callback_called = false;
  callback_call_count = 0;
  struct timespec pre, post;

  {
  Idle idle(&OnSourceCallbackContinue);
  clock_gettime(CLOCK_MONOTONIC, &pre);
  idle.removed.connect([&] (unsigned int id) { clock_gettime(CLOCK_MONOTONIC, &post); });

  Utils::WaitForTimeoutMSec(100);
  EXPECT_TRUE(idle.IsRunning());
  }

  EXPECT_TRUE(callback_called);
  EXPECT_GT(callback_call_count, 1);
  DeltaTime time_delta = unity::TimeUtil::TimeDelta(&post, &pre);
  EXPECT_GE(time_delta, 100);
  EXPECT_LT(time_delta, 200);
}

TEST(TestGLibIdle, Removal)
{
  callback_called = false;
  callback_call_count = 0;
  bool removed_called = false;

  Idle idle(&OnSourceCallbackContinue);
  idle.removed.connect([&] (unsigned int id) { removed_called = true; });
  idle.Remove();

  Utils::WaitUntilMSec([&idle] {return idle.IsRunning();}, false, 300);

  EXPECT_NE(idle.Id(), 0);
  EXPECT_TRUE(removed_called);
  EXPECT_FALSE(callback_called);
  EXPECT_EQ(callback_call_count, 0);
}

TEST(TestGLibIdle, Running)
{
  callback_called = false;
  callback_call_count = 0;

  Idle idle;
  EXPECT_FALSE(idle.IsRunning());

  idle.Run(&OnSourceCallbackStop);
  EXPECT_TRUE(idle.IsRunning());

  Utils::WaitUntilMSec([&idle] {return idle.IsRunning();}, false, 20000);
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(callback_call_count, 1);
}

TEST(TestGLibIdle, RemoveOnCallback)
{
  bool local_callback_called = false;
  unsigned int local_callback_call_count = 0;

  Idle idle([&] {
    local_callback_called = true;
    ++local_callback_call_count;
    idle.Remove();

    // this function would be called more than once if we had not removed the source.
    return true;
  });

  Utils::WaitForTimeoutMSec(100);

  ASSERT_EQ(idle.IsRunning(), false);
  EXPECT_EQ(local_callback_called, true);
  EXPECT_EQ(local_callback_call_count, 1);
}

TEST(TestGLibIdle, RemovePtrOnCallback)
{
  bool local_callback_called = false;
  unsigned int local_callback_call_count = 0;

  Source::UniquePtr idle(new Idle([&] {
    local_callback_called = true;
    ++local_callback_call_count;
    idle.reset();

    // this function would be called more than once if we had not removed the source.
    return true;
  }));

  Utils::WaitForTimeoutMSec(100);

  ASSERT_EQ(idle, nullptr);
  EXPECT_EQ(local_callback_called, true);
  EXPECT_EQ(local_callback_call_count, 1);
}

// Test GLibSource Manager

class MockSourceManager : public SourceManager
{
public:
  SourcesMap GetSources()
  {
    return sources_;
  }
};

TEST(TestGLibSourceManager, Construction)
{
  SourceManager manager;
}

TEST(TestGLibSourceManager, AddingAnonymousSources)
{
  MockSourceManager manager;

  manager.Add(new Timeout(1));
  manager.Add(new Timeout(1, &OnSourceCallbackContinue));
  manager.Add(new Idle());
  manager.Add(new Idle(&OnSourceCallbackContinue));

  EXPECT_EQ(manager.GetSources().size(), 4);
}

TEST(TestGLibSourceManager, AddingNamedSources)
{
  MockSourceManager manager;

  Source* timeout_1 = new Timeout(1);
  manager.Add(timeout_1, "timeout-1");
  ASSERT_EQ(manager.GetSource("timeout-1").get(), timeout_1);

  Source* timeout_2 = new Timeout(1, &OnSourceCallbackContinue);
  manager.Add(timeout_2, "timeout-2");
  ASSERT_EQ(manager.GetSource("timeout-2").get(), timeout_2);

  Source* idle_1 = new Idle();
  manager.Add(idle_1, "idle-1");
  ASSERT_EQ(manager.GetSource("idle-1").get(), idle_1);

  Source* idle_2 = new Idle(&OnSourceCallbackContinue);
  manager.Add(idle_2, "idle-2");
  ASSERT_EQ(manager.GetSource("idle-2").get(), idle_2);

  EXPECT_EQ(manager.GetSources().size(), 4);
}

TEST(TestGLibSourceManager, AddingDuplicatedSources)
{
  MockSourceManager manager;
  bool ret = false;

  Source::Ptr timeout(new Timeout(1));

  ret = manager.Add(timeout);
  EXPECT_EQ(manager.GetSource(timeout->Id()), timeout);
  EXPECT_EQ(ret, true);

  ret = manager.Add(timeout);
  EXPECT_EQ(manager.GetSource(timeout->Id()), timeout);
  EXPECT_EQ(ret, false);

  EXPECT_EQ(manager.GetSources().size(), 1);
}

TEST(TestGLibSourceManager, AddingDuplicatedNamedSources)
{
  MockSourceManager manager;
  bool ret = false;

  Source::Ptr timeout_1(new Timeout(1, &OnSourceCallbackContinue));
  Source::Ptr timeout_2(new Timeout(2));

  ret = manager.Add(timeout_1, "timeout");
  EXPECT_EQ(manager.GetSource("timeout"), timeout_1);
  EXPECT_EQ(ret, true);

  ret = manager.Add(timeout_2, "timeout");
  EXPECT_EQ(manager.GetSource("timeout"), timeout_2);
  EXPECT_EQ(ret, true);

  EXPECT_FALSE(timeout_1->IsRunning());
  EXPECT_EQ(manager.GetSources().size(), 1);
}

TEST(TestGLibSourceManager, AddingTimeouts)
{
  MockSourceManager manager;

  auto timeout1 = manager.AddTimeout(1);
  auto timeout2 = manager.AddTimeout(1, &OnSourceCallbackContinue);

  EXPECT_EQ(manager.GetSources().size(), 2);
  EXPECT_FALSE(timeout1->IsRunning());
  EXPECT_TRUE(timeout2->IsRunning());
}

TEST(TestGLibSourceManager, AddingIdles)
{
  MockSourceManager manager;

  auto idle1 = manager.AddIdle();
  auto idle2 = manager.AddIdle(&OnSourceCallbackContinue);

  EXPECT_EQ(manager.GetSources().size(), 2);
  EXPECT_FALSE(idle1->IsRunning());
  EXPECT_TRUE(idle2->IsRunning());
}

TEST(TestGLibSourceManager, RemovingSourcesById)
{
  MockSourceManager manager;

  Source::Ptr timeout1(new Timeout(1));
  Source::Ptr timeout2(new Timeout(2, &OnSourceCallbackContinue));
  Source::Ptr idle1(new Idle());
  Source::Ptr idle2(new Idle(&OnSourceCallbackContinue));

  manager.Add(timeout1);
  manager.Add(timeout2);
  manager.Add(idle1);
  manager.Add(idle2);

  manager.Remove(timeout1->Id());
  EXPECT_FALSE(timeout1->IsRunning());
  EXPECT_EQ(manager.GetSources().size(), 3);

  manager.Remove(timeout2->Id());
  EXPECT_FALSE(timeout2->IsRunning());
  EXPECT_EQ(manager.GetSource(timeout2->Id()), nullptr);
  EXPECT_EQ(manager.GetSources().size(), 2);

  manager.Remove(idle1->Id());
  EXPECT_FALSE(idle1->IsRunning());
  EXPECT_EQ(manager.GetSources().size(), 1);

  manager.Remove(idle2->Id());
  EXPECT_FALSE(idle2->IsRunning());
  EXPECT_EQ(manager.GetSource(idle2->Id()), nullptr);
  EXPECT_EQ(manager.GetSources().size(), 0);
}

TEST(TestGLibSourceManager, RemovingSourcesByNick)
{
  MockSourceManager manager;

  Source::Ptr timeout1(new Timeout(1));
  Source::Ptr timeout2(new Timeout(2, &OnSourceCallbackContinue));
  Source::Ptr idle1(new Idle());
  Source::Ptr idle2(new Idle(&OnSourceCallbackContinue));

  manager.Add(timeout1, "timeout-1");
  manager.Add(timeout2, "timeout-2");
  manager.Add(idle1, "idle-1");
  manager.Add(idle2, "idle-2");

  manager.Remove("timeout-1");
  EXPECT_FALSE(timeout1->IsRunning());
  EXPECT_EQ(manager.GetSource("timeout-1"), nullptr);
  EXPECT_EQ(manager.GetSources().size(), 3);

  manager.Remove("timeout-2");
  EXPECT_FALSE(timeout2->IsRunning());
  EXPECT_EQ(manager.GetSource("timeout-2"), nullptr);
  EXPECT_EQ(manager.GetSources().size(), 2);

  manager.Remove("idle-1");
  EXPECT_FALSE(idle1->IsRunning());
  EXPECT_EQ(manager.GetSource("idle-1"), nullptr);
  EXPECT_EQ(manager.GetSources().size(), 1);

  manager.Remove("idle-2");
  EXPECT_FALSE(idle2->IsRunning());
  EXPECT_EQ(manager.GetSource("idle-2"), nullptr);
  EXPECT_EQ(manager.GetSources().size(), 0);
}

TEST(TestGLibSourceManager, RemovesOnDisconnection)
{
  MockSourceManager manager;

  Source::Ptr timeout(new Timeout(1, &OnSourceCallbackContinue));
  Source::Ptr idle(new Idle(&OnSourceCallbackContinue));

  manager.Add(timeout);
  manager.Add(idle);

  ASSERT_EQ(manager.GetSources().size(), 2);
  EXPECT_EQ(timeout, manager.GetSource(timeout->Id()));
  EXPECT_EQ(idle, manager.GetSource(idle->Id()));

  timeout->Remove();
  EXPECT_EQ(manager.GetSources().size(), 1);
  EXPECT_EQ(manager.GetSource(timeout->Id()), nullptr);

  idle->Remove();
  EXPECT_EQ(manager.GetSources().size(), 0);
  EXPECT_EQ(manager.GetSource(idle->Id()), nullptr);
}

TEST(TestGLibSourceManager, DisconnectsOnRemoval)
{
  Source::Ptr timeout(new Timeout(1, &OnSourceCallbackContinue));
  Source::Ptr idle(new Idle(&OnSourceCallbackContinue));

  {
    SourceManager manager;
    manager.Add(timeout);
    manager.Add(idle, "test-idle");
    ASSERT_TRUE(timeout->IsRunning());
    ASSERT_TRUE(idle->IsRunning());
  }

  EXPECT_FALSE(timeout->IsRunning());
  EXPECT_FALSE(idle->IsRunning());
}

TEST(TestGLibSourceManager, RemoveSourceOnCallback)
{
  SourceManager manager;
  bool local_callback_called = false;
  unsigned int local_callback_call_count = 0;

  Source::Ptr idle(new Idle());
  manager.Add(idle, "test-idle");
  idle->Run([&] {
    local_callback_called = true;
    ++local_callback_call_count;
    manager.Remove("test-idle");
    // this function would be called more than once if we had not removed the source.
    return true;
  });

  Utils::WaitForTimeoutMSec(100);

  ASSERT_EQ(idle->IsRunning(), false);
  EXPECT_EQ(local_callback_called, true);
  EXPECT_EQ(local_callback_call_count, 1);
}

}
