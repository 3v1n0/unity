/*
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

#include <gtest/gtest.h>
#include "CompoundGestureRecognizer.h"
#include "FakeGestureEvent.h"

using namespace unity;

class CompoundGestureRecognizerTest : public ::testing::Test
{
 public:
  CompoundGestureRecognizerTest()
  {
    fake_event.gesture_id = 0;
    fake_event.timestamp = 12345; // some arbitrary, big value
  }

  RecognitionResult PerformTap()
  {
    fake_event.type = nux::EVENT_GESTURE_BEGIN;
    fake_event.gesture_id += 1;
    fake_event.gesture_classes = nux::TOUCH_GESTURE;
    if (gesture_recognizer.GestureEvent(fake_event.ToGestureEvent())
        != RecognitionResult::NONE)
      ADD_FAILURE();

    fake_event.type = nux::EVENT_GESTURE_UPDATE;
    fake_event.timestamp += 0.2 * CompoundGestureRecognizer::MAX_TAP_TIME;
    if (gesture_recognizer.GestureEvent(fake_event.ToGestureEvent())
        != RecognitionResult::NONE)
      ADD_FAILURE();

    fake_event.type = nux::EVENT_GESTURE_END;
    fake_event.timestamp += 0.6 * CompoundGestureRecognizer::MAX_TAP_TIME;
    return gesture_recognizer.GestureEvent(fake_event.ToGestureEvent());
  }


  CompoundGestureRecognizer gesture_recognizer;
  nux::FakeGestureEvent fake_event;
};

TEST_F(CompoundGestureRecognizerTest, DoubleTap)
{
  // First tap
  ASSERT_EQ(RecognitionResult::NONE, PerformTap());

  // Second tap
  fake_event.timestamp += 0.6 * CompoundGestureRecognizer::MAX_TIME_BETWEEN_GESTURES;
  ASSERT_EQ(RecognitionResult::DOUBLE_TAP_RECOGNIZED, PerformTap());
}

/*
  If too much time passes between two consecutive taps,
  it's not considered a double tap
 */
TEST_F(CompoundGestureRecognizerTest, Tap_BigInterval_Tap)
{
  // First tap
  ASSERT_EQ(RecognitionResult::NONE, PerformTap());

  // Second tap
  fake_event.timestamp += 2 * CompoundGestureRecognizer::MAX_TIME_BETWEEN_GESTURES;
  ASSERT_EQ(RecognitionResult::NONE, PerformTap());
}

TEST_F(CompoundGestureRecognizerTest, Tap_BigInterval_DoubleTap)
{
  // First tap
  ASSERT_EQ(RecognitionResult::NONE, PerformTap());

  // Second tap
  fake_event.timestamp += 2 * CompoundGestureRecognizer::MAX_TIME_BETWEEN_GESTURES;
  ASSERT_EQ(RecognitionResult::NONE, PerformTap());

  // Third tap
  fake_event.timestamp += 0.5 * CompoundGestureRecognizer::MAX_TIME_BETWEEN_GESTURES;
  ASSERT_EQ(RecognitionResult::DOUBLE_TAP_RECOGNIZED, PerformTap());
}

TEST_F(CompoundGestureRecognizerTest, TapAndDrag)
{
  // First tap
  ASSERT_EQ(RecognitionResult::NONE, PerformTap());

  // Drag

  fake_event.type = nux::EVENT_GESTURE_BEGIN;
  fake_event.gesture_id = 1;
  fake_event.timestamp += 0.6 * CompoundGestureRecognizer::MAX_TIME_BETWEEN_GESTURES;
  ASSERT_EQ(RecognitionResult::NONE,
      gesture_recognizer.GestureEvent(fake_event.ToGestureEvent()));

  fake_event.type = nux::EVENT_GESTURE_UPDATE;
  fake_event.gesture_classes = nux::TOUCH_GESTURE | nux::DRAG_GESTURE;
  fake_event.timestamp += 0.2 * CompoundGestureRecognizer::MAX_TIME_BETWEEN_GESTURES;
  ASSERT_EQ(RecognitionResult::NONE,
      gesture_recognizer.GestureEvent(fake_event.ToGestureEvent()));

  fake_event.type = nux::EVENT_GESTURE_END;
  fake_event.timestamp += 0.6 * CompoundGestureRecognizer::MAX_TAP_TIME;
  ASSERT_EQ(RecognitionResult::NONE,
      gesture_recognizer.GestureEvent(fake_event.ToGestureEvent()));
}

TEST_F(CompoundGestureRecognizerTest, TapAndHold)
{
  // Tap

  ASSERT_EQ(RecognitionResult::NONE, PerformTap());

  // Hold

  fake_event.type = nux::EVENT_GESTURE_BEGIN;
  fake_event.gesture_id = 1;
  fake_event.timestamp += 0.6 * CompoundGestureRecognizer::MAX_TIME_BETWEEN_GESTURES;
  ASSERT_EQ(RecognitionResult::NONE,
      gesture_recognizer.GestureEvent(fake_event.ToGestureEvent()));

  fake_event.type = nux::EVENT_GESTURE_UPDATE;
  fake_event.timestamp += 0.2 * CompoundGestureRecognizer::MAX_TAP_TIME;
  ASSERT_EQ(RecognitionResult::NONE,
      gesture_recognizer.GestureEvent(fake_event.ToGestureEvent()));

  fake_event.type = nux::EVENT_GESTURE_UPDATE;
  fake_event.timestamp += 1.0 * CompoundGestureRecognizer::HOLD_TIME;
  ASSERT_EQ(RecognitionResult::TAP_AND_HOLD_RECOGNIZED,
      gesture_recognizer.GestureEvent(fake_event.ToGestureEvent()));

  fake_event.type = nux::EVENT_GESTURE_END;
  fake_event.timestamp += 0.3 * CompoundGestureRecognizer::HOLD_TIME;
  ASSERT_EQ(RecognitionResult::NONE,
      gesture_recognizer.GestureEvent(fake_event.ToGestureEvent()));
}

TEST_F(CompoundGestureRecognizerTest, Tap_ShortHold_DoubleTap)
{
  // Tap

  ASSERT_EQ(RecognitionResult::NONE, PerformTap());

  // Short hold
  // Too long for a tap and too short for a hold.

  fake_event.type = nux::EVENT_GESTURE_BEGIN;
  fake_event.gesture_id = 1;
  fake_event.timestamp += 0.6 * CompoundGestureRecognizer::MAX_TIME_BETWEEN_GESTURES;
  ASSERT_EQ(RecognitionResult::NONE,
      gesture_recognizer.GestureEvent(fake_event.ToGestureEvent()));

  fake_event.type = nux::EVENT_GESTURE_UPDATE;
  fake_event.timestamp += (CompoundGestureRecognizer::MAX_TAP_TIME +
                           CompoundGestureRecognizer::HOLD_TIME) / 2;
  ASSERT_EQ(RecognitionResult::NONE,
      gesture_recognizer.GestureEvent(fake_event.ToGestureEvent()));

  fake_event.type = nux::EVENT_GESTURE_END;
  fake_event.timestamp += 1;
  ASSERT_EQ(RecognitionResult::NONE,
      gesture_recognizer.GestureEvent(fake_event.ToGestureEvent()));

  // Verify that the state machine went back to its initial state
  // by checking that a subsequent double tap gets recognized normally

  fake_event.timestamp += 0.5 * CompoundGestureRecognizer::MAX_TIME_BETWEEN_GESTURES;
  ASSERT_EQ(RecognitionResult::NONE, PerformTap());

  fake_event.timestamp += 0.5 * CompoundGestureRecognizer::MAX_TIME_BETWEEN_GESTURES;
  ASSERT_EQ(RecognitionResult::DOUBLE_TAP_RECOGNIZED, PerformTap());
}
