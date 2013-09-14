/*
 * CompoundGestureRecognizer.h
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

#ifndef COMPOUND_GESTURE_RECOGNIZER_H
#define COMPOUND_GESTURE_RECOGNIZER_H

#include <sigc++/sigc++.h>

namespace nux
{
  class GestureEvent;
}

namespace unity
{

enum class RecognitionResult
{
  NONE,
  DOUBLE_TAP_RECOGNIZED, /*! Returned when a double-tap is recognized */
  TAP_AND_HOLD_RECOGNIZED, /*!< Returned when a "tap and hold" is recognized
                             At this point the user is still "holding". I.e.,
                             his fingers are still on the touchscreen or
                             trackpad. */
};

class CompoundGestureRecognizerPrivate;

/*!
  Recognizes compound gestures. I.e. high level gestures that are maded up by
  two sequencial regular gestures (like a tap followed by a second tap).
 */
class CompoundGestureRecognizer
{
  public:
    // in milliseconds
    static const int MAX_TIME_BETWEEN_GESTURES = 600;
    static const int MAX_TAP_TIME = 300;
    static const int HOLD_TIME = 600;

    CompoundGestureRecognizer();
    virtual ~CompoundGestureRecognizer();

    virtual RecognitionResult GestureEvent(nux::GestureEvent const& event);

  private:
    CompoundGestureRecognizerPrivate* p;
};

} // namespace unity

#endif // COMPOUND_GESTURE_RECOGNIZER_H
