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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#include <gmock/gmock.h>
#include <glib.h>
#include "unity-shared/AnimationUtils.h"

namespace
{

using namespace unity::animation;

TEST(TestAnimationUtils, StartOrReverseNotRunning)
{
  na::AnimateValue<double> animation;
  double start = g_random_double_range(std::numeric_limits<double>::min(), -1);
  double finish = g_random_double_range(1, std::numeric_limits<double>::max());

  StartOrReverse<double>(animation, start, finish);
  EXPECT_DOUBLE_EQ(start, animation.GetStartValue());
  EXPECT_DOUBLE_EQ(finish, animation.GetFinishValue());
  EXPECT_EQ(na::Animation::State::Running, animation.CurrentState());
}

TEST(TestAnimationUtils, StartOrReverseKeepsRunning)
{
  na::AnimateValue<double> animation(3);
  double start = g_random_double_range(std::numeric_limits<double>::min(), -1);
  double finish = g_random_double_range(1, std::numeric_limits<double>::max());

  animation.SetStartValue(start).SetFinishValue(finish).Start();
  ASSERT_EQ(na::Animation::State::Running, animation.CurrentState());
  animation.Advance(1);
  ASSERT_EQ(1, animation.CurrentTimePosition());

  StartOrReverse<double>(animation, start, finish);
  EXPECT_DOUBLE_EQ(start, animation.GetStartValue());
  EXPECT_DOUBLE_EQ(finish, animation.GetFinishValue());
  EXPECT_EQ(na::Animation::State::Running, animation.CurrentState());
  EXPECT_EQ(1, animation.CurrentTimePosition());
}

TEST(TestAnimationUtils, StartOrReverseReversesRunning)
{
  na::AnimateValue<double> animation(3);
  double start = g_random_double_range(std::numeric_limits<double>::min(), -1);
  double finish = g_random_double_range(1, std::numeric_limits<double>::max());

  animation.SetStartValue(start).SetFinishValue(finish).Start();
  ASSERT_EQ(na::Animation::State::Running, animation.CurrentState());
  animation.Advance(1);
  ASSERT_EQ(1, animation.CurrentTimePosition());

  StartOrReverse<double>(animation, finish, start);
  EXPECT_DOUBLE_EQ(finish, animation.GetStartValue());
  EXPECT_DOUBLE_EQ(start, animation.GetFinishValue());
  EXPECT_EQ(na::Animation::State::Running, animation.CurrentState());
  EXPECT_EQ(2, animation.CurrentTimePosition());
}

TEST(TestAnimationUtils, StartOrReverseInValidRunning)
{
  na::AnimateValue<double> animation(3);
  double start = g_random_double_range(std::numeric_limits<double>::min(), -1);
  double finish = g_random_double_range(1, std::numeric_limits<double>::max());

  animation.SetStartValue(start+1).SetFinishValue(finish-1).Start();
  ASSERT_EQ(na::Animation::State::Running, animation.CurrentState());
  animation.Advance(1);
  ASSERT_EQ(1, animation.CurrentTimePosition());

  StartOrReverse<double>(animation, finish, start);
  EXPECT_DOUBLE_EQ(finish, animation.GetStartValue());
  EXPECT_DOUBLE_EQ(start, animation.GetFinishValue());
  EXPECT_EQ(na::Animation::State::Running, animation.CurrentState());
  EXPECT_EQ(0, animation.CurrentTimePosition());
}

TEST(TestAnimationUtils, StartOrReverseDoubleForward)
{
  na::AnimateValue<double> animation;
  StartOrReverse(animation, Direction::FORWARD);

  EXPECT_DOUBLE_EQ(0.0f, animation.GetStartValue());
  EXPECT_DOUBLE_EQ(1.0f, animation.GetFinishValue());
  EXPECT_EQ(na::Animation::State::Running, animation.CurrentState());
}

TEST(TestAnimationUtils, StartOrReverseDoubleBackward)
{
  na::AnimateValue<double> animation;
  StartOrReverse(animation, Direction::BACKWARD);

  EXPECT_DOUBLE_EQ(1.0f, animation.GetStartValue());
  EXPECT_DOUBLE_EQ(0.0f, animation.GetFinishValue());
  EXPECT_EQ(na::Animation::State::Running, animation.CurrentState());
}

TEST(TestAnimationUtils, StartOrReverseIfDoubleTrue)
{
  na::AnimateValue<double> animation;

  StartOrReverseIf(animation, true);
  EXPECT_DOUBLE_EQ(0.0f, animation.GetStartValue());
  EXPECT_DOUBLE_EQ(1.0f, animation.GetFinishValue());
  EXPECT_EQ(na::Animation::State::Running, animation.CurrentState());
}

TEST(TestAnimationUtils, StartOrReverseIfDoubleFalse)
{
  na::AnimateValue<double> animation;
  StartOrReverseIf(animation, false);

  EXPECT_DOUBLE_EQ(1.0f, animation.GetStartValue());
  EXPECT_DOUBLE_EQ(0.0f, animation.GetFinishValue());
  EXPECT_EQ(na::Animation::State::Running, animation.CurrentState());
}

TEST(TestAnimationUtils, StartOrReverseFloatForward)
{
  na::AnimateValue<float> animation;
  StartOrReverse(animation, Direction::FORWARD);

  EXPECT_FLOAT_EQ(0.0f, animation.GetStartValue());
  EXPECT_FLOAT_EQ(1.0f, animation.GetFinishValue());
  EXPECT_EQ(na::Animation::State::Running, animation.CurrentState());
}

TEST(TestAnimationUtils, StartOrReverseFloatBackward)
{
  na::AnimateValue<float> animation;
  StartOrReverse(animation, Direction::BACKWARD);

  EXPECT_FLOAT_EQ(1.0f, animation.GetStartValue());
  EXPECT_FLOAT_EQ(0.0f, animation.GetFinishValue());
  EXPECT_EQ(na::Animation::State::Running, animation.CurrentState());
}

TEST(TestAnimationUtils, StartOrReverseIfFloatTrue)
{
  na::AnimateValue<float> animation;
  StartOrReverseIf(animation, true);

  EXPECT_FLOAT_EQ(0.0f, animation.GetStartValue());
  EXPECT_FLOAT_EQ(1.0f, animation.GetFinishValue());
  EXPECT_EQ(na::Animation::State::Running, animation.CurrentState());
}

TEST(TestAnimationUtils, StartOrReverseIfFloatFalse)
{
  na::AnimateValue<float> animation;
  StartOrReverseIf(animation, false);

  EXPECT_FLOAT_EQ(1.0f, animation.GetStartValue());
  EXPECT_FLOAT_EQ(0.0f, animation.GetFinishValue());
  EXPECT_EQ(na::Animation::State::Running, animation.CurrentState());
}

TEST(TestAnimationUtils, StartOrReverseIntForward)
{
  na::AnimateValue<int> animation;
  StartOrReverse(animation, Direction::FORWARD);

  EXPECT_EQ(0, animation.GetStartValue());
  EXPECT_EQ(100, animation.GetFinishValue());
  EXPECT_EQ(na::Animation::State::Running, animation.CurrentState());
}

TEST(TestAnimationUtils, StartOrReverseIntBackward)
{
  na::AnimateValue<int> animation;
  StartOrReverse(animation, Direction::BACKWARD);

  EXPECT_EQ(100, animation.GetStartValue());
  EXPECT_EQ(0, animation.GetFinishValue());
  EXPECT_EQ(na::Animation::State::Running, animation.CurrentState());
}

TEST(TestAnimationUtils, StartOrReverseIfIntTrue)
{
  na::AnimateValue<int> animation;
  StartOrReverseIf(animation, true);

  EXPECT_EQ(0, animation.GetStartValue());
  EXPECT_EQ(100, animation.GetFinishValue());
  EXPECT_EQ(na::Animation::State::Running, animation.CurrentState());
}

TEST(TestAnimationUtils, StartOrReverseIfIntFalse)
{
  na::AnimateValue<int> animation;
  StartOrReverseIf(animation, false);

  EXPECT_EQ(100, animation.GetStartValue());
  EXPECT_EQ(0, animation.GetFinishValue());
  EXPECT_EQ(na::Animation::State::Running, animation.CurrentState());
}

TEST(TestAnimationUtils, StartOrReverseExchanges)
{
  na::AnimateValue<double> animation;

  for (int i = 0; i < 10; ++i)
  {
    StartOrReverse(animation, (i % 2) ? Direction::FORWARD : Direction::BACKWARD);
    ASSERT_DOUBLE_EQ((i % 2) ? 0.0f : 1.0f, animation.GetStartValue());
    ASSERT_DOUBLE_EQ((i % 2) ? 1.0f : 0.0f, animation.GetFinishValue());
    ASSERT_EQ(na::Animation::State::Running, animation.CurrentState());
  }
}

TEST(TestAnimationUtils, StartOrReverseIfExchanges)
{
  na::AnimateValue<double> animation;

  for (int i = 0; i < 10; ++i)
  {
    StartOrReverseIf(animation, (i % 2));
    ASSERT_DOUBLE_EQ((i % 2) ? 0.0f : 1.0f, animation.GetStartValue());
    ASSERT_DOUBLE_EQ((i % 2) ? 1.0f : 0.0f, animation.GetFinishValue());
    ASSERT_EQ(na::Animation::State::Running, animation.CurrentState());
  }
}

TEST(TestAnimationUtils, GetDirection)
{
  na::AnimateValue<double> animation;
  EXPECT_EQ(Direction::FORWARD, GetDirection(animation));
}

TEST(TestAnimationUtils, GetDirectionForward)
{
  na::AnimateValue<double> animation;
  animation.SetStartValue(0).SetFinishValue(10);
  EXPECT_EQ(Direction::FORWARD, GetDirection(animation));
}

TEST(TestAnimationUtils, GetDirectionBackward)
{
  na::AnimateValue<double> animation;
  animation.SetStartValue(10).SetFinishValue(0);
  EXPECT_EQ(Direction::BACKWARD, GetDirection(animation));
}

TEST(TestAnimationUtils, GetDirectionStartedReversed)
{
  na::AnimateValue<double> animation;
  StartOrReverse(animation, Direction::BACKWARD);
  EXPECT_EQ(Direction::BACKWARD, GetDirection(animation));

  StartOrReverse(animation, Direction::FORWARD);
  EXPECT_EQ(Direction::FORWARD, GetDirection(animation));
}

} // Namespace
