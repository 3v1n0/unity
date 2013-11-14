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

template <typename T> struct TestAnimationUtilsFloating : testing::Test {};
typedef testing::Types<float, double> FloatingTypes;
TYPED_TEST_CASE(TestAnimationUtilsFloating, FloatingTypes);

template <typename T> struct TestAnimationUtilsIntegers : testing::Test {};
typedef testing::Types<int, unsigned> IntegerTypes;
TYPED_TEST_CASE(TestAnimationUtilsIntegers, IntegerTypes);

template <typename T> struct TestAnimationUtils : testing::Test {};
typedef testing::Types<float, double, int, unsigned> AllTypes;
TYPED_TEST_CASE(TestAnimationUtils, AllTypes);

template <typename T> T GetRandomValue(T min, T max) { return g_random_double_range(min, max); }
template <> int GetRandomValue(int min, int max) { return g_random_int_range(min, max); }
template <> unsigned GetRandomValue(unsigned min, unsigned max)
{
  if (max > std::numeric_limits<int>::max())
  {
    max = std::numeric_limits<int>::max();
    if (max <= min)
      min = max/2;
  }

  return GetRandomValue<int>(min, max);
}

TYPED_TEST(TestAnimationUtilsFloating, StartValueForDirection)
{
  EXPECT_DOUBLE_EQ(0.0f, StartValueForDirection<TypeParam>(Direction::FORWARD));
  EXPECT_DOUBLE_EQ(1.0f, StartValueForDirection<TypeParam>(Direction::BACKWARD));
}

TYPED_TEST(TestAnimationUtilsIntegers, StartValueForDirection)
{
  EXPECT_EQ(0, StartValueForDirection<TypeParam>(Direction::FORWARD));
  EXPECT_EQ(100, StartValueForDirection<TypeParam>(Direction::BACKWARD));
}

TYPED_TEST(TestAnimationUtilsFloating, FinishValueForDirection)
{
  EXPECT_DOUBLE_EQ(1.0f, FinishValueForDirection<TypeParam>(Direction::FORWARD));
  EXPECT_DOUBLE_EQ(0.0f, FinishValueForDirection<TypeParam>(Direction::BACKWARD));
}

TYPED_TEST(TestAnimationUtilsIntegers, FinishValueForDirection)
{
  EXPECT_EQ(100, FinishValueForDirection<TypeParam>(Direction::FORWARD));
  EXPECT_EQ(0, FinishValueForDirection<TypeParam>(Direction::BACKWARD));
}

TYPED_TEST(TestAnimationUtils, StartNotRunning)
{
  na::AnimateValue<TypeParam> animation;
  TypeParam start = GetRandomValue<TypeParam>(std::numeric_limits<TypeParam>::min(), 2);
  TypeParam finish = GetRandomValue<TypeParam>(3, std::numeric_limits<TypeParam>::max());

  Start<TypeParam>(animation, start, finish);
  EXPECT_DOUBLE_EQ(start, animation.GetStartValue());
  EXPECT_DOUBLE_EQ(finish, animation.GetFinishValue());
  EXPECT_EQ(na::Animation::State::Running, animation.CurrentState());
}

TYPED_TEST(TestAnimationUtils, StartRunning)
{
  na::AnimateValue<TypeParam> animation(3);
  TypeParam start = GetRandomValue<TypeParam>(std::numeric_limits<TypeParam>::min(), 2);
  TypeParam finish = GetRandomValue<TypeParam>(3, std::numeric_limits<TypeParam>::max());

  animation.SetStartValue(start+1).SetFinishValue(finish-1).Start();
  ASSERT_EQ(na::Animation::State::Running, animation.CurrentState());
  animation.Advance(1);
  ASSERT_EQ(1, animation.CurrentTimePosition());

  Start<TypeParam>(animation, finish, start);
  EXPECT_DOUBLE_EQ(finish, animation.GetStartValue());
  EXPECT_DOUBLE_EQ(start, animation.GetFinishValue());
  EXPECT_EQ(na::Animation::State::Running, animation.CurrentState());
  EXPECT_EQ(0, animation.CurrentTimePosition());
}

TYPED_TEST(TestAnimationUtils, StartOrReverseNotRunning)
{
  na::AnimateValue<TypeParam> animation;
  TypeParam start = GetRandomValue<TypeParam>(std::numeric_limits<TypeParam>::min(), 2);
  TypeParam finish = GetRandomValue<TypeParam>(3, std::numeric_limits<TypeParam>::max());

  StartOrReverse<TypeParam>(animation, start, finish);
  EXPECT_DOUBLE_EQ(start, animation.GetStartValue());
  EXPECT_DOUBLE_EQ(finish, animation.GetFinishValue());
  EXPECT_EQ(na::Animation::State::Running, animation.CurrentState());
}

TYPED_TEST(TestAnimationUtils, StartOrReverseKeepsRunning)
{
  na::AnimateValue<TypeParam> animation(3);
  TypeParam start = GetRandomValue<TypeParam>(std::numeric_limits<TypeParam>::min(), 2);
  TypeParam finish = GetRandomValue<TypeParam>(3, std::numeric_limits<TypeParam>::max());

  animation.SetStartValue(start).SetFinishValue(finish).Start();
  ASSERT_EQ(na::Animation::State::Running, animation.CurrentState());
  animation.Advance(1);
  ASSERT_EQ(1, animation.CurrentTimePosition());

  StartOrReverse<TypeParam>(animation, start, finish);
  EXPECT_DOUBLE_EQ(start, animation.GetStartValue());
  EXPECT_DOUBLE_EQ(finish, animation.GetFinishValue());
  EXPECT_EQ(na::Animation::State::Running, animation.CurrentState());
  EXPECT_EQ(1, animation.CurrentTimePosition());
}

TYPED_TEST(TestAnimationUtils, StartOrReverseReversesRunning)
{
  na::AnimateValue<TypeParam> animation(3);
  TypeParam start = GetRandomValue<TypeParam>(std::numeric_limits<TypeParam>::min(), 2);
  TypeParam finish = GetRandomValue<TypeParam>(3, std::numeric_limits<TypeParam>::max());

  animation.SetStartValue(start).SetFinishValue(finish).Start();
  ASSERT_EQ(na::Animation::State::Running, animation.CurrentState());
  animation.Advance(1);
  ASSERT_EQ(1, animation.CurrentTimePosition());

  StartOrReverse<TypeParam>(animation, finish, start);
  EXPECT_DOUBLE_EQ(finish, animation.GetStartValue());
  EXPECT_DOUBLE_EQ(start, animation.GetFinishValue());
  EXPECT_EQ(na::Animation::State::Running, animation.CurrentState());
  EXPECT_EQ(2, animation.CurrentTimePosition());
}

TYPED_TEST(TestAnimationUtils, StartOrReverseInValidRunning)
{
  na::AnimateValue<TypeParam> animation(3);
  TypeParam start = GetRandomValue<TypeParam>(std::numeric_limits<TypeParam>::min(), 2);
  TypeParam finish = GetRandomValue<TypeParam>(3, std::numeric_limits<TypeParam>::max());

  animation.SetStartValue(start+1).SetFinishValue(finish-1).Start();
  ASSERT_EQ(na::Animation::State::Running, animation.CurrentState());
  animation.Advance(1);
  ASSERT_EQ(1, animation.CurrentTimePosition());

  StartOrReverse<TypeParam>(animation, finish, start);
  EXPECT_DOUBLE_EQ(finish, animation.GetStartValue());
  EXPECT_DOUBLE_EQ(start, animation.GetFinishValue());
  EXPECT_EQ(na::Animation::State::Running, animation.CurrentState());
  EXPECT_EQ(0, animation.CurrentTimePosition());
}

TYPED_TEST(TestAnimationUtils, StartForward)
{
  na::AnimateValue<TypeParam> animation;
  Start(animation, Direction::FORWARD);

  EXPECT_DOUBLE_EQ(StartValueForDirection<TypeParam>(Direction::FORWARD), animation.GetStartValue());
  EXPECT_DOUBLE_EQ(FinishValueForDirection<TypeParam>(Direction::FORWARD), animation.GetFinishValue());
  EXPECT_EQ(na::Animation::State::Running, animation.CurrentState());
}

TYPED_TEST(TestAnimationUtils, StartBackward)
{
  na::AnimateValue<TypeParam> animation;
  Start(animation, Direction::BACKWARD);

  EXPECT_DOUBLE_EQ(StartValueForDirection<TypeParam>(Direction::BACKWARD), animation.GetStartValue());
  EXPECT_DOUBLE_EQ(FinishValueForDirection<TypeParam>(Direction::BACKWARD), animation.GetFinishValue());
  EXPECT_EQ(na::Animation::State::Running, animation.CurrentState());
}

TYPED_TEST(TestAnimationUtils, StartOrReverseForward)
{
  na::AnimateValue<TypeParam> animation;
  StartOrReverse(animation, Direction::FORWARD);

  EXPECT_DOUBLE_EQ(StartValueForDirection<TypeParam>(Direction::FORWARD), animation.GetStartValue());
  EXPECT_DOUBLE_EQ(FinishValueForDirection<TypeParam>(Direction::FORWARD), animation.GetFinishValue());
  EXPECT_EQ(na::Animation::State::Running, animation.CurrentState());
}

TYPED_TEST(TestAnimationUtils, StartOrReverseBackward)
{
  na::AnimateValue<TypeParam> animation;
  StartOrReverse(animation, Direction::BACKWARD);

  EXPECT_DOUBLE_EQ(StartValueForDirection<TypeParam>(Direction::BACKWARD), animation.GetStartValue());
  EXPECT_DOUBLE_EQ(FinishValueForDirection<TypeParam>(Direction::BACKWARD), animation.GetFinishValue());
  EXPECT_EQ(na::Animation::State::Running, animation.CurrentState());
}

TYPED_TEST(TestAnimationUtils, StartOrReverseIfTrue)
{
  na::AnimateValue<TypeParam> animation;

  StartOrReverseIf(animation, true);
  EXPECT_DOUBLE_EQ(StartValueForDirection<TypeParam>(Direction::FORWARD), animation.GetStartValue());
  EXPECT_DOUBLE_EQ(FinishValueForDirection<TypeParam>(Direction::FORWARD), animation.GetFinishValue());
  EXPECT_EQ(na::Animation::State::Running, animation.CurrentState());
}

TYPED_TEST(TestAnimationUtils, StartOrReverseIfFalse)
{
  na::AnimateValue<TypeParam> animation;
  StartOrReverseIf(animation, false);

  EXPECT_DOUBLE_EQ(StartValueForDirection<TypeParam>(Direction::BACKWARD), animation.GetStartValue());
  EXPECT_DOUBLE_EQ(FinishValueForDirection<TypeParam>(Direction::BACKWARD), animation.GetFinishValue());
  EXPECT_EQ(na::Animation::State::Running, animation.CurrentState());
}

TYPED_TEST(TestAnimationUtils, StartOrReverseExchanges)
{
  na::AnimateValue<TypeParam> animation;

  for (int i = 0; i < 10; ++i)
  {
    auto direction = (i % 2) ? Direction::FORWARD : Direction::BACKWARD;
    StartOrReverse(animation, direction);
    ASSERT_DOUBLE_EQ(StartValueForDirection<TypeParam>(direction), animation.GetStartValue());
    ASSERT_DOUBLE_EQ(FinishValueForDirection<TypeParam>(direction), animation.GetFinishValue());
    ASSERT_EQ(na::Animation::State::Running, animation.CurrentState());
  }
}

TYPED_TEST(TestAnimationUtils, StartOrReverseIfExchanges)
{
  na::AnimateValue<TypeParam> animation;

  for (int i = 0; i < 10; ++i)
  {
    bool val = (i % 2) ? true : false;
    auto direction = val ? Direction::FORWARD : Direction::BACKWARD;
    StartOrReverseIf(animation, val);
    ASSERT_DOUBLE_EQ(StartValueForDirection<TypeParam>(direction), animation.GetStartValue());
    ASSERT_DOUBLE_EQ(FinishValueForDirection<TypeParam>(direction), animation.GetFinishValue());
    ASSERT_EQ(na::Animation::State::Running, animation.CurrentState());
  }
}

TYPED_TEST(TestAnimationUtils, GetDirection)
{
  na::AnimateValue<TypeParam> animation;
  EXPECT_EQ(Direction::FORWARD, GetDirection(animation));
}

TYPED_TEST(TestAnimationUtils, GetDirectionForward)
{
  na::AnimateValue<TypeParam> animation;
  animation.SetStartValue(0).SetFinishValue(10);
  EXPECT_EQ(Direction::FORWARD, GetDirection(animation));
}

TYPED_TEST(TestAnimationUtils, GetDirectionBackward)
{
  na::AnimateValue<TypeParam> animation;
  animation.SetStartValue(10).SetFinishValue(0);
  EXPECT_EQ(Direction::BACKWARD, GetDirection(animation));
}

TYPED_TEST(TestAnimationUtils, GetDirectionStartedReversed)
{
  na::AnimateValue<TypeParam> animation;
  StartOrReverse(animation, Direction::BACKWARD);
  EXPECT_EQ(Direction::BACKWARD, GetDirection(animation));

  StartOrReverse(animation, Direction::FORWARD);
  EXPECT_EQ(Direction::FORWARD, GetDirection(animation));
}

TYPED_TEST(TestAnimationUtils, SetValue)
{
  na::AnimateValue<TypeParam> animation;
  TypeParam value = GetRandomValue(std::numeric_limits<TypeParam>::min(), std::numeric_limits<TypeParam>::max());
  SetValue(animation, value);

  EXPECT_DOUBLE_EQ(value, animation.GetStartValue());
  EXPECT_DOUBLE_EQ(value, animation.GetFinishValue());
  EXPECT_DOUBLE_EQ(value, animation.GetCurrentValue());
  EXPECT_EQ(na::Animation::State::Stopped, animation.CurrentState());
}

TYPED_TEST(TestAnimationUtils, SetValueForward)
{
  na::AnimateValue<TypeParam> animation;
  SetValue(animation, Direction::FORWARD);

  EXPECT_DOUBLE_EQ(FinishValueForDirection<TypeParam>(Direction::FORWARD), animation.GetStartValue());
  EXPECT_DOUBLE_EQ(FinishValueForDirection<TypeParam>(Direction::FORWARD), animation.GetFinishValue());
  EXPECT_DOUBLE_EQ(FinishValueForDirection<TypeParam>(Direction::FORWARD), animation.GetCurrentValue());
  EXPECT_EQ(na::Animation::State::Stopped, animation.CurrentState());
}

TYPED_TEST(TestAnimationUtils, SetValueBackward)
{
  na::AnimateValue<TypeParam> animation;
  SetValue(animation, Direction::BACKWARD);

  EXPECT_DOUBLE_EQ(FinishValueForDirection<TypeParam>(Direction::BACKWARD), animation.GetStartValue());
  EXPECT_DOUBLE_EQ(FinishValueForDirection<TypeParam>(Direction::BACKWARD), animation.GetFinishValue());
  EXPECT_DOUBLE_EQ(FinishValueForDirection<TypeParam>(Direction::BACKWARD), animation.GetCurrentValue());
  EXPECT_EQ(na::Animation::State::Stopped, animation.CurrentState());
}

TYPED_TEST(TestAnimationUtils, Skip)
{
  na::AnimateValue<TypeParam> animation;
  TypeParam start = GetRandomValue<TypeParam>(std::numeric_limits<TypeParam>::min(), 2);
  TypeParam finish = GetRandomValue<TypeParam>(3, std::numeric_limits<TypeParam>::max());
  animation.SetStartValue(start).SetFinishValue(finish);
  Skip(animation);

  EXPECT_DOUBLE_EQ(finish, animation.GetCurrentValue());
  EXPECT_DOUBLE_EQ(start, animation.GetStartValue());
  EXPECT_DOUBLE_EQ(finish, animation.GetFinishValue());
  EXPECT_EQ(na::Animation::State::Stopped, animation.CurrentState());
}

} // Namespace
