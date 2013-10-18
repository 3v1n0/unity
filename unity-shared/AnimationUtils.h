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

#ifndef UNITY_ANIMATION_UTILS
#define UNITY_ANIMATION_UTILS

#include <NuxCore/Animation.h>

namespace na = nux::animation;

namespace unity
{
namespace animation
{

enum class Direction
{
  FORWARD,
  BACKWARD
};

template <typename VALUE>
inline Direction GetDirection(na::AnimateValue<VALUE> const& animation)
{
  if (animation.GetFinishValue() < animation.GetStartValue())
    return Direction::BACKWARD;

  return Direction::FORWARD;
}

template <typename VALUE>
inline VALUE StartValueForDirection(Direction dir)
{
  return (dir == Direction::FORWARD) ? 0.0f : 1.0f;
}

template <>
inline unsigned StartValueForDirection(Direction dir)
{
  return (dir == Direction::FORWARD) ? 0 : 100;
}

template <>
inline int StartValueForDirection(Direction dir)
{
  return StartValueForDirection<unsigned>(dir);
}

template <typename VALUE>
inline VALUE FinishValueForDirection(Direction dir)
{
  return StartValueForDirection<VALUE>(dir == Direction::FORWARD ? Direction::BACKWARD : Direction::FORWARD);
}

template <class VALUE_TYPE>
void Start(na::AnimateValue<VALUE_TYPE>& animation, VALUE_TYPE start, VALUE_TYPE finish);

template <typename VALUE>
inline void Start(na::AnimateValue<VALUE>& animation, Direction dir)
{
  Start<VALUE>(animation, StartValueForDirection<VALUE>(dir), FinishValueForDirection<VALUE>(dir));
}

template <class VALUE_TYPE>
void StartOrReverse(na::AnimateValue<VALUE_TYPE>& animation, VALUE_TYPE start, VALUE_TYPE finish);

template <typename VALUE>
inline void StartOrReverse(na::AnimateValue<VALUE>& animation, Direction dir)
{
  StartOrReverse<VALUE>(animation, StartValueForDirection<VALUE>(dir), FinishValueForDirection<VALUE>(dir));
}

template <typename VALUE>
inline void StartOrReverseIf(na::AnimateValue<VALUE>& animation, bool condition)
{
  StartOrReverse(animation, condition ? Direction::FORWARD : Direction::BACKWARD);
}

template <typename VALUE>
inline void SetValue(na::AnimateValue<VALUE>& animation, VALUE const& value)
{
  Start(animation, value, value);
}

template <typename VALUE>
inline void SetValue(na::AnimateValue<VALUE>& animation, Direction dir)
{
  SetValue(animation, FinishValueForDirection<VALUE>(dir));
}

template <typename VALUE>
inline void Skip(na::AnimateValue<VALUE>& animation)
{
  SetValue(animation, animation.GetFinishValue());
}

} // animation namespace
} // unity namespace

#include "AnimationUtils-inl.h"

#endif // UNITY_ANIMATION_UTILS

