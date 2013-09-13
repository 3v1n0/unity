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

template <class VALUE_TYPE>
void StartOrReverse(na::AnimateValue<VALUE_TYPE>& animation, VALUE_TYPE start, VALUE_TYPE finish)
{
  if (animation.CurrentState() == na::Animation::State::Running)
  {
    if (animation.GetStartValue() == finish && animation.GetFinishValue() == start)
    {
      animation.Reverse();
    }
    else if (animation.GetStartValue() != start || animation.GetFinishValue() != finish)
    {
      animation.Stop();
      animation.SetStartValue(start).SetFinishValue(finish).Start();
    }
  }
  else
  {
    animation.SetStartValue(start).SetFinishValue(finish).Start();
  }
}

enum class Direction
{
  FORWARD,
  BACKWARD
};

template <typename VALUE>
void StartOrReverse(na::AnimateValue<VALUE>& animation, Direction dir)
{
  StartOrReverse<VALUE>(animation, (dir == Direction::FORWARD) ? 0.0f : 1.0f,
                                   (dir == Direction::FORWARD) ? 1.0f : 0.0f);
}

template <>
void StartOrReverse(na::AnimateValue<int>& animation, Direction dir)
{
  StartOrReverse<int>(animation, (dir == Direction::FORWARD) ? 0 : 100,
                                 (dir == Direction::FORWARD) ? 100 : 0);
}

template <typename VALUE>
void StartOrReverseIf(na::AnimateValue<VALUE>& animation, bool condition)
{
  StartOrReverse(animation, condition ? Direction::FORWARD : Direction::BACKWARD);
}

} // animation namespace
} // unity namespace

#endif // UNITY_ANIMATION_UTILS

