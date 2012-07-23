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
 * Authored by: Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 */

#include <gtest/gtest.h>

#include "Animator.h"
#include "test_utils.h"

using namespace std;
using namespace unity;

namespace
{

class TestAnimator : public ::testing::Test
{
public:
  TestAnimator() :
   test_animator_(100),
   n_steps_(0),
   current_progress_(0.0f),
   got_update_(false),
   half_reached_(false),
   started_(false),
   stopped_(false),
   ended_(false)
  {
    test_animator_.animation_started.connect([this]() {
      started_ = true;
    });

    test_animator_.animation_ended.connect([this]() {
      ended_ = true;
    });

    test_animator_.animation_stopped.connect([this](double progress) {
      stopped_ = true;
    });

    test_animator_.animation_updated.connect([this](double progress) {
      n_steps_++;
      current_progress_ = progress;
      got_update_ = true;

      if (progress >= 0.5f)
        half_reached_ = true;
    });
  }

protected:
  void ResetValues()
  {
    n_steps_ = 0;
    current_progress_ = 0.0f;
    started_ = false;
    stopped_ = false;
    ended_ = false;
    got_update_ = false;
    half_reached_ = false;
  }

  Animator test_animator_;
  unsigned int n_steps_;
  double current_progress_;
  bool got_update_;
  bool half_reached_;
  bool started_;
  bool stopped_;
  bool ended_;
};

TEST_F(TestAnimator, ConstructDestroy)
{
  bool stopped = false;
  double progress = 0.0f;
  
  {
    Animator tmp_animator(200, 25);

    EXPECT_EQ(tmp_animator.GetDuration(), 200);
    EXPECT_EQ(tmp_animator.GetRate(), 25);

    bool got_update = false;
    tmp_animator.animation_updated.connect([&progress, &got_update](double p) {
      progress = p;
      got_update = true;
    });

    tmp_animator.animation_stopped.connect([&stopped](double p) {
      stopped = true;
    });

    tmp_animator.Start();

    Utils::WaitUntil(got_update);

    EXPECT_EQ(tmp_animator.IsRunning(), true);
    EXPECT_GT(progress, 0.0f);
    EXPECT_EQ(stopped, false);
  }

  EXPECT_EQ(stopped, true);
}

TEST_F(TestAnimator, SetGetValues)
{
  test_animator_.SetRate(30);
  EXPECT_EQ(test_animator_.GetRate(), 30);

  test_animator_.SetDuration(100);
  EXPECT_EQ(test_animator_.GetDuration(), 100);

  EXPECT_EQ(test_animator_.GetProgress(), 0.0f);
  EXPECT_EQ(test_animator_.IsRunning(), false);
}

TEST_F(TestAnimator, SimulateStep)
{
  test_animator_.DoStep();
  EXPECT_EQ(test_animator_.IsRunning(), false);
  EXPECT_EQ(n_steps_, 1);
  EXPECT_GT(test_animator_.GetProgress(), 0.0f);
  ResetValues();
}

TEST_F(TestAnimator, SimulateAnimation)
{
  test_animator_.SetRate(20);
  test_animator_.SetDuration(200);
  long long start_time = g_get_monotonic_time() / 1000;
  test_animator_.Start();

  EXPECT_EQ(started_, true);
  EXPECT_EQ(test_animator_.IsRunning(), true);
  
  Utils::WaitUntil(got_update_);
  EXPECT_GT(test_animator_.GetProgress(), 0.0f);
  EXPECT_EQ(test_animator_.GetProgress(), current_progress_);
  EXPECT_EQ(n_steps_, 1);

  Utils::WaitUntil(ended_);
  EXPECT_EQ(stopped_, false);
  EXPECT_EQ(ended_, true);

  unsigned int expected_steps = (test_animator_.GetDuration() / 1000.0f) * test_animator_.GetRate();
  EXPECT_EQ(n_steps_, ceil(expected_steps));

  long long actual_time = g_get_monotonic_time() / 1000;
  long long expected_end = start_time+test_animator_.GetDuration();
  EXPECT_GE(actual_time, expected_end);
  EXPECT_LE(actual_time-expected_end, test_animator_.GetRate()*2);

  ResetValues();
}

TEST_F(TestAnimator, SimulateStoppedAnimation)
{
  test_animator_.SetRate(30);
  test_animator_.SetDuration(100);
  test_animator_.Start();
  EXPECT_EQ(started_, true);
  EXPECT_EQ(test_animator_.IsRunning(), true);

  Utils::WaitUntil(half_reached_);
  EXPECT_GT(test_animator_.GetProgress(), 0.5f);
  EXPECT_EQ(test_animator_.GetProgress(), current_progress_);
  EXPECT_EQ(test_animator_.IsRunning(), true);

  test_animator_.Stop();
  EXPECT_EQ(test_animator_.IsRunning(), false);
  EXPECT_LT(test_animator_.GetProgress(), 1.0f);
  EXPECT_EQ(stopped_, true);
  EXPECT_EQ(ended_, true);

  ResetValues();
}

TEST_F(TestAnimator, SimulateStoppedAndContinueAnimation)
{
  test_animator_.SetRate(30);
  test_animator_.SetDuration(100);
  test_animator_.Start();
  EXPECT_EQ(started_, true);
  EXPECT_EQ(test_animator_.IsRunning(), true);

  Utils::WaitUntil(half_reached_);
  test_animator_.Stop();

  EXPECT_LT(test_animator_.GetProgress(), 1.0f);
  EXPECT_EQ(stopped_, true);
  EXPECT_EQ(ended_, true);
  stopped_ = false;
  ended_ = false;

  long long start_time = g_get_monotonic_time() / 1000;
  long long expected_end = start_time + (test_animator_.GetDuration() / current_progress_);
  test_animator_.Start(test_animator_.GetProgress());
  Utils::WaitUntil(ended_);
  EXPECT_EQ(stopped_, false);
  EXPECT_EQ(ended_, true);

  long long actual_time = g_get_monotonic_time() / 1000;
  EXPECT_LT(expected_end-actual_time, test_animator_.GetDuration());

  ResetValues();
}

TEST_F(TestAnimator, SimulateOneTimeDurationStart)
{
  unsigned int default_duration = 100;

  test_animator_.SetRate(30);
  test_animator_.SetDuration(default_duration);

  unsigned int one_time_duration = 75;
  test_animator_.Start(one_time_duration);
  EXPECT_EQ(started_, true);
  EXPECT_EQ(test_animator_.IsRunning(), true);

  Utils::WaitUntil(half_reached_);
  EXPECT_LT(test_animator_.GetProgress(), 1.0f);
  EXPECT_EQ(test_animator_.GetDuration(), one_time_duration);
  EXPECT_EQ(ended_, false);

  Utils::WaitUntil(ended_);
  EXPECT_EQ(stopped_, false);
  EXPECT_EQ(ended_, true);

  EXPECT_EQ(test_animator_.GetDuration(), default_duration);

  ResetValues();
}

TEST_F(TestAnimator, SimulateOneTimeDurationStartStop)
{
  unsigned int default_duration = 100;

  test_animator_.SetRate(30);
  test_animator_.SetDuration(default_duration);

  unsigned int one_time_duration = 75;
  test_animator_.Start(one_time_duration);
  EXPECT_EQ(started_, true);
  EXPECT_EQ(test_animator_.IsRunning(), true);

  Utils::WaitUntil(half_reached_);
  EXPECT_EQ(test_animator_.GetDuration(), one_time_duration);
  EXPECT_EQ(ended_, false);

  test_animator_.Stop();
  EXPECT_EQ(stopped_, true);
  EXPECT_EQ(ended_, true);
  EXPECT_EQ(test_animator_.GetDuration(), default_duration);

  ResetValues();
}

TEST_F(TestAnimator, SimulateZeroDuration)
{
  test_animator_.SetRate(30);
  test_animator_.SetDuration(0);

  EXPECT_EQ(started_, false);
  EXPECT_EQ(ended_, false);
  EXPECT_EQ(test_animator_.IsRunning(), false);

  long long start_time = g_get_monotonic_time() / 1000;
  test_animator_.Start();
  EXPECT_EQ(started_, true);

  Utils::WaitUntil(ended_);
  EXPECT_EQ(ended_, true);

  long long end_time = g_get_monotonic_time() / 1000;
  EXPECT_LT(end_time - start_time, test_animator_.GetRate()*2);
}


} // Namespace
