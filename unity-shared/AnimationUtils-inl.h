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

#ifndef UNITY_ANIMATION_UTILS_INL
#define UNITY_ANIMATION_UTILS_INL

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

} // animation namespace
} // unity namespace

#endif // UNITY_ANIMATION_UTILS_INL
