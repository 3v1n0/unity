// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010 Canonical Ltd
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
 * aint with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Jason Smith <jason.smith@canonical.com>
 */

#ifndef GEISADAPTER_H
#define GEISADAPTER_H

/* Compiz */
#include <sigc++/sigc++.h>
#include <geis/geis.h>
#include <Nux/Nux.h>

class GeisAdapter : public sigc::trackable
{
public:
  static GeisAdapter& Instance();

  GeisAdapter();
  ~GeisAdapter();

  void Run();

  typedef struct _GeisTapData
  {
    int id;
    int device_id;
    Window window;
    int touches;
    int timestamp;
    float focus_x;
    float focus_y;
    int tap_length_ms;
    float position_x;
    float position_y;
    float bound_x1;
    float bound_y1;
    float bound_x2;
    float bound_y2;
  } GeisTapData;

  typedef struct _GeisDragData
  {
    int id;
    int device_id;
    Window window;
    int touches;
    int timestamp;
    float focus_x;
    float focus_y;
    float delta_x;
    float delta_y;
    float velocity_x;
    float velocity_y;
    float position_x;
    float position_y;
    float bound_x1;
    float bound_y1;
    float bound_x2;
    float bound_y2;
  } GeisDragData;

  typedef struct _GeisRotateData
  {
    int id;
    int device_id;
    Window window;
    int touches;
    int timestamp;
    float focus_x;
    float focus_y;
    float angle;
    float angle_delta;
    float angle_velocity;
    float bound_x1;
    float bound_y1;
    float bound_x2;
    float bound_y2;
  } GeisRotateData;

  typedef struct _GeisPinchData
  {
    int id;
    int device_id;
    Window window;
    int touches;
    int timestamp;
    float focus_x;
    float focus_y;
    float radius;
    float radius_delta;
    float radius_velocity;
    float bound_x1;
    float bound_y1;
    float bound_x2;
    float bound_y2;
  } GeisPinchData;

  typedef struct _GeisTouchData
  {
    int id;
    int device_id;
    Window window;
    int touches;
    int timestamp;
    float focus_x;
    float focus_y;
    float bound_x1;
    float bound_y1;
    float bound_x2;
    float bound_y2;
  } GeisTouchData;

  sigc::signal<void, GeisTapData*> tap;

  sigc::signal<void, GeisDragData*> drag_start;
  sigc::signal<void, GeisDragData*> drag_update;
  sigc::signal<void, GeisDragData*> drag_finish;

  sigc::signal<void, GeisRotateData*> rotate_start;
  sigc::signal<void, GeisRotateData*> rotate_update;
  sigc::signal<void, GeisRotateData*> rotate_finish;

  sigc::signal<void, GeisPinchData*> pinch_start;
  sigc::signal<void, GeisPinchData*> pinch_update;
  sigc::signal<void, GeisPinchData*> pinch_finish;

  sigc::signal<void, GeisTouchData*> touch_start;
  sigc::signal<void, GeisTouchData*> touch_update;
  sigc::signal<void, GeisTouchData*> touch_finish;
protected:
  static gboolean OnWatchIn(GIOChannel* source, GIOCondition condition, gpointer data);

  static void InputDeviceAdded(void* cookie, GeisInputDeviceId device_id, void* attrs);
  static void InputDeviceChanged(void* cookie, GeisInputDeviceId device_id, void* attrs);
  static void InputDeviceRemoved(void* cookie, GeisInputDeviceId device_id, void* attrs);

  static void GestureAdded(void* cookie, GeisGestureType gesture_type, GeisGestureId gesture_id, GeisSize attr_count, GeisGestureAttr* attrs);
  static void GestureRemoved(void* cookie, GeisGestureType gesture_type, GeisGestureId gesture_id, GeisSize attr_count, GeisGestureAttr* attrs);

  static void GestureStart(void* cookie, GeisGestureType gesture_type, GeisGestureId gesture_id, GeisSize attr_count, GeisGestureAttr* attrs);
  static void GestureUpdate(void* cookie, GeisGestureType gesture_type, GeisGestureId gesture_id, GeisSize attr_count, GeisGestureAttr* attrs);
  static void GestureFinish(void* cookie, GeisGestureType gesture_type, GeisGestureId gesture_id, GeisSize attr_count, GeisGestureAttr* attrs);

  GeisTapData*     ProcessTapGesture(GeisSize attr_count, GeisGestureAttr* attrs);
  GeisDragData*    ProcessDragGesture(GeisSize attr_count, GeisGestureAttr* attrs);
  GeisPinchData*   ProcessPinchGesture(GeisSize attr_count, GeisGestureAttr* attrs);
  GeisRotateData* ProcessRotateGesture(GeisSize attr_count, GeisGestureAttr* attrs);
  GeisTouchData*   ProcessTouchGesture(GeisSize attr_count, GeisGestureAttr* attrs);

private:
  void RegisterRootInstance();

  GeisInstance _root_instance;
  guint _watch_id;
};

#endif
