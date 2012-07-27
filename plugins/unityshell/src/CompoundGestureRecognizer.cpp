/*
 * CompoundGestureRecognizer.cpp
 * This file is part of Unity
 *
 * Copyright (C) 2012 - Canonical Ltd.
 *
 * Unity is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Unity is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Unity; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#include "CompoundGestureRecognizer.h"
#include <NuxGraphics/GestureEvent.h>

CompoundGestureRecognizer::CompoundGestureRecognizer()
  : state_(State::WaitingFirstTapBegin)
{
}

RecognitionResult CompoundGestureRecognizer::GestureEvent(const nux::GestureEvent &event)
{
  switch (state_)
  {
    case State::WaitingFirstTapBegin:
      return WaitingFirstTapBegin(event);
      break;
    case State::WaitingFirstTapEnd:
      return WaitingFirstTapEnd(event);
      break;
    case State::WaitingSecondGestureBegin:
      return WaitingSecondGestureBegin(event);
      break;
    default: // State::RecognizingSecondGesture:
      return RecognizingSecondGesture(event);
  }
}

RecognitionResult CompoundGestureRecognizer::WaitingFirstTapBegin(const nux::GestureEvent &event)
{
  if (event.type == nux::EVENT_GESTURE_BEGIN)
  {
    first_gesture.id = event.GetGestureId();
    first_gesture.begin_time = event.GetTimestamp();
    state_ = State::WaitingFirstTapEnd;
  }
  return RecognitionResult::NONE;
}

RecognitionResult CompoundGestureRecognizer::WaitingFirstTapEnd(const nux::GestureEvent &event)
{
  if (event.type != nux::EVENT_GESTURE_END)
    return RecognitionResult::NONE;

  if (first_gesture.id != event.GetGestureId())
  {
    ResetStateMachine();
    return RecognitionResult::NONE;
  }

  if (event.GetGestureClasses() != nux::TOUCH_GESTURE)
  {
    // some other gesture class such as drag or pinch was also recognized,
    // meaning that the touch points moved too much and therefore it cannot
    // be a tap.
    ResetStateMachine();
    return RecognitionResult::NONE;
  }

  first_gesture.end_time = event.GetTimestamp();
  if (first_gesture.Duration() > MAX_TAP_TIME)
  {
    // can't be a tap. it took too long
    ResetStateMachine();
    return RecognitionResult::NONE;
  }

  state_ = State::WaitingSecondGestureBegin;

  return RecognitionResult::NONE;
}

RecognitionResult CompoundGestureRecognizer::WaitingSecondGestureBegin(const nux::GestureEvent &event)
{

  if (event.type != nux::EVENT_GESTURE_BEGIN)
  {
    // that's not right. there shouldn't be any ongoing gesture now.
    ResetStateMachine();
    return RecognitionResult::NONE;
  }

  if (event.GetGestureClasses() != nux::TOUCH_GESTURE)
  {
    ResetStateMachine();
    return RecognitionResult::NONE;
  }

  int interval = event.GetTimestamp() - first_gesture.end_time;
  if (interval > MAX_TIME_BETWEEN_GESTURES)
  {
    ResetStateMachine();
    // consider it as the possible first tap of a new compound gesture
    GestureEvent(event);
    return RecognitionResult::NONE;
  }

  second_gesture.id = event.GetGestureId();
  second_gesture.begin_time = event.GetTimestamp();

  state_ = State::RecognizingSecondGesture;

  return RecognitionResult::NONE;
}

RecognitionResult CompoundGestureRecognizer::RecognizingSecondGesture(const nux::GestureEvent &event)
{
  if (event.GetGestureId() != second_gesture.id)
  {
    // no simultaneous gestures
    ResetStateMachine();
    return RecognitionResult::NONE;
  }

  if (event.GetGestureClasses() != nux::TOUCH_GESTURE)
  {
    // some other gesture class such as drag or pinch was also recognized,
    // meaning that the touch points moved too much and therefore it cannot
    // be a tap or a hold
    ResetStateMachine();
    return RecognitionResult::NONE;
  }

  RecognitionResult result = RecognitionResult::NONE;

  if (event.type == nux::EVENT_GESTURE_UPDATE)
  {
    if (event.GetTimestamp() - second_gesture.begin_time >= HOLD_TIME)
    {
      result = RecognitionResult::TAP_AND_HOLD_RECOGNIZED;
      ResetStateMachine();
    }
  }
  else
  {
    g_assert(event.type == nux::EVENT_GESTURE_END);
    second_gesture.end_time = event.GetTimestamp();

    if (second_gesture.Duration() <= MAX_TAP_TIME)
    {
      result = RecognitionResult::DOUBLE_TAP_RECOGNIZED;
    }
    ResetStateMachine();
  }

  return result;
}

void CompoundGestureRecognizer::ResetStateMachine()
{
  first_gesture.Clear();
  second_gesture.Clear();
  state_ = State::WaitingFirstTapBegin;
}
