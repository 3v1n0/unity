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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Daniel d'Andrada <daniel.dandrada@canonical.com>
 */

#include "CompoundGestureRecognizer.h"
#include <NuxCore/Logger.h>
#include <NuxGraphics/GestureEvent.h>

DECLARE_LOGGER(logger, "unity.gesture.recognizer");

namespace unity
{
  class CompoundGestureRecognizerPrivate
  {
   public:
    CompoundGestureRecognizerPrivate();

    enum class State
    {
      WaitingFirstTapBegin,
      WaitingFirstTapEnd,
      WaitingSecondGestureBegin,
      RecognizingSecondGesture
    };

    RecognitionResult GestureEvent(nux::GestureEvent const& event);

    RecognitionResult WaitingFirstTapBegin(nux::GestureEvent const& event);
    RecognitionResult WaitingFirstTapEnd(nux::GestureEvent const& event);
    RecognitionResult WaitingSecondGestureBegin(nux::GestureEvent const& event);
    RecognitionResult RecognizingSecondGesture(nux::GestureEvent const& event);
    void ResetStateMachine();

    State state;

    class GestureInfo
    {
      public:
        GestureInfo() {Clear();}
        int begin_time;
        int end_time;
        int id;
        int Duration() const {return end_time - begin_time;}
        void Clear() {begin_time = end_time = id = -1;}
    };
    GestureInfo first_gesture;
    GestureInfo second_gesture;
  };
}

using namespace unity;

///////////////////////////////////////////
// private class

CompoundGestureRecognizerPrivate::CompoundGestureRecognizerPrivate()
  : state(State::WaitingFirstTapBegin)
{
}

RecognitionResult CompoundGestureRecognizerPrivate::GestureEvent(nux::GestureEvent const& event)
{
  switch (state)
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

RecognitionResult CompoundGestureRecognizerPrivate::WaitingFirstTapBegin(nux::GestureEvent const& event)
{
  if (event.type == nux::EVENT_GESTURE_BEGIN)
  {
    first_gesture.id = event.GetGestureId();
    first_gesture.begin_time = event.GetTimestamp();
    state = State::WaitingFirstTapEnd;
  }
  return RecognitionResult::NONE;
}

RecognitionResult CompoundGestureRecognizerPrivate::WaitingFirstTapEnd(nux::GestureEvent const& event)
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
  if (first_gesture.Duration() > CompoundGestureRecognizer::MAX_TAP_TIME)
  {
    // can't be a tap. it took too long
    ResetStateMachine();
    return RecognitionResult::NONE;
  }

  state = State::WaitingSecondGestureBegin;

  return RecognitionResult::NONE;
}

RecognitionResult CompoundGestureRecognizerPrivate::WaitingSecondGestureBegin(
    nux::GestureEvent const& event)
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
  if (interval > CompoundGestureRecognizer::MAX_TIME_BETWEEN_GESTURES)
  {
    ResetStateMachine();
    // consider it as the possible first tap of a new compound gesture
    GestureEvent(event);
    return RecognitionResult::NONE;
  }

  second_gesture.id = event.GetGestureId();
  second_gesture.begin_time = event.GetTimestamp();

  state = State::RecognizingSecondGesture;

  return RecognitionResult::NONE;
}

RecognitionResult CompoundGestureRecognizerPrivate::RecognizingSecondGesture(
    nux::GestureEvent const& event)
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
    if (event.GetTimestamp() - second_gesture.begin_time >= CompoundGestureRecognizer::HOLD_TIME)
    {
      result = RecognitionResult::TAP_AND_HOLD_RECOGNIZED;
      ResetStateMachine();
    }
  }
  else if (event.type == nux::EVENT_GESTURE_END)
  {
    second_gesture.end_time = event.GetTimestamp();

    if (second_gesture.Duration() <= CompoundGestureRecognizer::MAX_TAP_TIME)
    {
      result = RecognitionResult::DOUBLE_TAP_RECOGNIZED;
    }
    ResetStateMachine();
  }
  else
  {
    // This really shouldn't happen.
    LOG_ERROR(logger) << "Unexpected gesture type."
      " CompoundGestureRecognizer left in an undefined state.";
  }

  return result;
}

void CompoundGestureRecognizerPrivate::ResetStateMachine()
{
  first_gesture.Clear();
  second_gesture.Clear();
  state = State::WaitingFirstTapBegin;
}

///////////////////////////////////////////
// public class

CompoundGestureRecognizer::CompoundGestureRecognizer()
  : p(new CompoundGestureRecognizerPrivate)
{
}

CompoundGestureRecognizer::~CompoundGestureRecognizer()
{
  delete p;
}

RecognitionResult CompoundGestureRecognizer::GestureEvent(nux::GestureEvent const& event)
{
  return p->GestureEvent(event);
}
