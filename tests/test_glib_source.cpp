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
  Timeout timeout(1000, Source::SourceCallback());
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
  struct timespec pre, post;

  Timeout timeout(100, &OnSourceCallbackStop);
  clock_gettime(CLOCK_MONOTONIC, &pre);
  timeout.removed.connect([&] (unsigned int id) { clock_gettime(CLOCK_MONOTONIC, &post); });

  Utils::WaitForTimeoutMSec(500);
  EXPECT_FALSE(timeout.IsRunning());
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(callback_call_count, 1);
  int time_delta = unity::TimeUtil::TimeDelta(&post, &pre);
  EXPECT_GE(time_delta, 100);
  EXPECT_LT(time_delta, 110);
}

TEST(TestGLibTimeout, MultipleShotsRun)
{
  callback_called = false;
  callback_call_count = 0;
  struct timespec pre, post;

  {
  Timeout timeout(100, &OnSourceCallbackContinue);
  clock_gettime(CLOCK_MONOTONIC, &pre);
  timeout.removed.connect([&] (unsigned int id) { clock_gettime(CLOCK_MONOTONIC, &post); });

  Utils::WaitForTimeoutMSec(650);
  EXPECT_TRUE(timeout.IsRunning());
  }

  EXPECT_TRUE(callback_called);
  EXPECT_EQ(callback_call_count, 6);
  int time_delta = unity::TimeUtil::TimeDelta(&post, &pre);
  EXPECT_GE(time_delta, 600);
  EXPECT_LT(time_delta, 700);
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

  Utils::WaitForTimeoutMSec(300);

  EXPECT_NE(timeout.Id(), 0);
  EXPECT_FALSE(timeout.IsRunning());
  EXPECT_TRUE(removed_called);
  EXPECT_FALSE(callback_called);
  EXPECT_EQ(callback_call_count, 0);
}

TEST(TestGLibTimeout, Running)
{
  callback_called = false;
  callback_call_count = 0;

  Timeout timeout(300);
  EXPECT_FALSE(timeout.IsRunning());

  timeout.Run(&OnSourceCallbackStop);
  EXPECT_TRUE(timeout.IsRunning());
  Utils::WaitForTimeoutMSec(600);

  EXPECT_FALSE(timeout.IsRunning());
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(callback_call_count, 1);
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

  Idle idle(&OnSourceCallbackStop);
  pre = g_get_monotonic_time();
  idle.removed.connect([&] (unsigned int id) { post = g_get_monotonic_time(); });

  Utils::WaitForTimeoutMSec(100);
  EXPECT_FALSE(idle.IsRunning());
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(callback_call_count, 1);
  EXPECT_LT(pre, post);
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
  int time_delta = unity::TimeUtil::TimeDelta(&post, &pre);
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

  Utils::WaitForTimeoutMSec(300);

  EXPECT_NE(idle.Id(), 0);
  EXPECT_FALSE(idle.IsRunning());
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
  Utils::WaitForTimeoutMSec(200);

  EXPECT_FALSE(idle.IsRunning());
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(callback_call_count, 1);
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

  Source *timeout_1 = new Timeout(1);
  manager.Add(timeout_1, "timeout-1");
  ASSERT_EQ(manager.GetSource("timeout-1").get(), timeout_1);

  Source *timeout_2 = new Timeout(1, &OnSourceCallbackContinue);
  manager.Add(timeout_2, "timeout-2");
  ASSERT_EQ(manager.GetSource("timeout-2").get(), timeout_2);

  Source *idle_1 = new Idle();
  manager.Add(idle_1, "idle-1");
  ASSERT_EQ(manager.GetSource("idle-1").get(), idle_1);

  Source *idle_2 = new Idle(&OnSourceCallbackContinue);
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

}
