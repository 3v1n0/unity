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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Jason Smith <jason.smith@canonical.com>
 */
 
#include <glib.h>
#include "GeisAdapter.h"

GeisAdapter * GeisAdapter::_default = 0;

/* static */
GeisAdapter *
GeisAdapter::Default (CompScreen *screen)
{
  if (!_default)
      return _default = new GeisAdapter (screen); // should be using a dictionary
  return _default;
}

GeisAdapter::GeisAdapter(CompScreen *screen)
{
  _screen = screen;
  RegisterRootInstance ();
}

GeisAdapter::~GeisAdapter()
{
}

void
GeisAdapter::Run ()
{
  int fd = -1;
  GeisStatus status;
  
  status = geis_configuration_get_value(_root_instance, GEIS_CONFIG_UNIX_FD, &fd);
  
  if (status != GEIS_STATUS_SUCCESS)
    return;
  
  _watch_id = g_io_add_watch (g_io_channel_unix_new (fd),
                              G_IO_IN,
                              &GeisAdapter::OnWatchIn,
                              this);
}

gboolean
GeisAdapter::OnWatchIn (GIOChannel *source, GIOCondition condition, gpointer data)
{
  GeisAdapter *self = static_cast<GeisAdapter*> (data);
  geis_event_dispatch (self->_root_instance);
  return true;
}

void
GeisAdapter::InputDeviceAdded(void *cookie, GeisInputDeviceId device_id, void *attrs)
{
}


void
GeisAdapter::InputDeviceChanged(void *cookie, GeisInputDeviceId device_id, void *attrs)
{
}


void
GeisAdapter::InputDeviceRemoved(void *cookie, GeisInputDeviceId device_id, void *attrs)
{
}

void
GeisAdapter::GestureAdded(void *cookie, GeisGestureType gesture_type, GeisGestureId gesture_id, GeisSize count, GeisGestureAttr *attrs)
{
}

void
GeisAdapter::GestureRemoved(void *cookie, GeisGestureType gesture_type, GeisGestureId gesture_id, GeisSize count, GeisGestureAttr *attrs)
{
}

void
GeisAdapter::GestureStart(void *cookie, GeisGestureType gesture_type, GeisGestureId gesture_id, GeisSize count, GeisGestureAttr *attrs)
{
  GeisAdapter *self = static_cast<GeisAdapter *> (cookie);
  
  if (gesture_type == 0)
  {
    GeisDragData *data = self->ProcessDragGesture (count, attrs);
    data->id = gesture_id;
    self->drag_start.emit (data);
    g_free (data);
  }
  else if (gesture_type == 2)
  {
    GeisRotateData *data = self->ProcessRotateGesture (count, attrs);
    data->id = gesture_id;
    self->rotate_start.emit (data);
    g_free (data);
  }
  else if (gesture_type == 1)
  {
    GeisPinchData *data = self->ProcessPinchGesture (count, attrs);
    data->id = gesture_id;
    self->pinch_start.emit (data);
    g_free (data);
  }
  else if (gesture_type == 15)
  {
    GeisTapData *data = self->ProcessTapGesture (count, attrs);
    data->id = gesture_id;
    self->tap.emit (data);
    g_free (data);
  }
}

void
GeisAdapter::GestureUpdate(void *cookie, GeisGestureType gesture_type, GeisGestureId gesture_id, GeisSize count, GeisGestureAttr *attrs)
{
  GeisAdapter *self = static_cast<GeisAdapter *> (cookie);
  
  if (gesture_type == 0)
  {
    GeisDragData *data = self->ProcessDragGesture (count, attrs);
    data->id = gesture_id;
    self->drag_update.emit (data);
    g_free (data);
  }
  else if (gesture_type == 2)
  {
    GeisRotateData *data = self->ProcessRotateGesture (count, attrs);
    data->id = gesture_id;
    self->rotate_update.emit (data);
    g_free (data);
  }
  else if (gesture_type == 1)
  {
    GeisPinchData *data = self->ProcessPinchGesture (count, attrs);
    data->id = gesture_id;
    self->pinch_update.emit (data);
    g_free (data);
  }
  else if (gesture_type == 15)
  {
    GeisTapData *data = self->ProcessTapGesture (count, attrs);
    data->id = gesture_id;
    self->tap.emit (data);
    g_free (data);
  }
}

void
GeisAdapter::GestureFinish(void *cookie, GeisGestureType gesture_type, GeisGestureId gesture_id, GeisSize count, GeisGestureAttr *attrs)
{
  GeisAdapter *self = static_cast<GeisAdapter *> (cookie);
  
  if (gesture_type == 0)
  {
    GeisDragData *data = self->ProcessDragGesture (count, attrs);
    data->id = gesture_id;
    self->drag_finish.emit (data);
    g_free (data);
  }
  else if (gesture_type == 2)
  {
    GeisRotateData *data = self->ProcessRotateGesture (count, attrs);
    data->id = gesture_id;
    self->rotate_finish.emit (data);
    g_free (data);
  }
  else if (gesture_type == 1)
  {
    GeisPinchData *data = self->ProcessPinchGesture (count, attrs);
    data->id = gesture_id;
    self->pinch_finish.emit (data);
    g_free (data);
  }
  else if (gesture_type == 15)
  {
    GeisTapData *data = self->ProcessTapGesture (count, attrs);
    data->id = gesture_id;
    self->tap.emit (data);
    g_free (data);
  }
}

GeisAdapter::GeisTapData * GeisAdapter::ProcessTapGesture (GeisSize count, GeisGestureAttr *attrs)
{
  GeisTapData *result = (GeisTapData*) g_malloc0 (sizeof (GeisTapData));

  int i;
  for (i = 0; i < (int) count; i++)
  {
    GeisGestureAttr attr = attrs[i];
    if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_DEVICE_ID))
      result->device_id = attr.integer_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_TIMESTAMP))
      result->timestamp = attr.integer_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_FOCUS_X))
      result->focus_x = attr.integer_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_FOCUS_Y))
      result->focus_y = attr.integer_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_TOUCHES))
      result->touches = attr.integer_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_TAP_TIME))
      result->tap_length_ms = attr.integer_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_POSITION_X))
      result->position_x = attr.float_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_POSITION_Y))
      result->position_y = attr.float_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_BOUNDINGBOX_X1))
      result->bound_x1 = attr.float_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_BOUNDINGBOX_Y1))
      result->bound_y1 = attr.float_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_BOUNDINGBOX_X2))
      result->bound_x2 = attr.float_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_BOUNDINGBOX_Y2))
      result->bound_y2 = attr.float_val;
  }
  
  return result;
}

GeisAdapter::GeisDragData * GeisAdapter::ProcessDragGesture (GeisSize count, GeisGestureAttr *attrs)
{
  GeisDragData *result = (GeisDragData*) g_malloc0 (sizeof (GeisDragData));

  int i;
  for (i = 0; i < (int) count; i++)
  {
    GeisGestureAttr attr = attrs[i];
    if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_DEVICE_ID))
      result->device_id = attr.integer_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_TIMESTAMP))
      result->timestamp = attr.integer_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_FOCUS_X))
      result->focus_x = attr.integer_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_FOCUS_Y))
      result->focus_y = attr.integer_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_TOUCHES))
      result->touches = attr.integer_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_POSITION_X))
      result->position_x = attr.float_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_POSITION_Y))
      result->position_y = attr.float_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_VELOCITY_X))
      result->velocity_x = attr.float_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_VELOCITY_Y))
      result->velocity_y = attr.float_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_DELTA_X))
      result->delta_x = attr.float_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_DELTA_Y))
      result->delta_y = attr.float_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_BOUNDINGBOX_X1))
      result->bound_x1 = attr.float_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_BOUNDINGBOX_Y1))
      result->bound_y1 = attr.float_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_BOUNDINGBOX_X2))
      result->bound_x2 = attr.float_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_BOUNDINGBOX_Y2))
      result->bound_y2 = attr.float_val;
  }
  
  return result;
}

GeisAdapter::GeisPinchData * GeisAdapter::ProcessPinchGesture (GeisSize count, GeisGestureAttr *attrs)
{
  GeisPinchData *result = (GeisPinchData*) g_malloc0 (sizeof (GeisPinchData));

  int i;
  for (i = 0; i < (int) count; i++)
  {
    GeisGestureAttr attr = attrs[i];
    if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_DEVICE_ID))
      result->device_id = attr.integer_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_TIMESTAMP))
      result->timestamp = attr.integer_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_FOCUS_X))
      result->focus_x = attr.integer_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_FOCUS_Y))
      result->focus_y = attr.integer_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_TOUCHES))
      result->touches = attr.integer_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_RADIUS))
      result->radius = attr.float_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_RADIUS_DELTA))
      result->radius_delta = attr.float_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_RADIAL_VELOCITY))
      result->radius_velocity = attr.float_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_BOUNDINGBOX_X1))
      result->bound_x1 = attr.float_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_BOUNDINGBOX_Y1))
      result->bound_y1 = attr.float_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_BOUNDINGBOX_X2))
      result->bound_x2 = attr.float_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_BOUNDINGBOX_Y2))
      result->bound_y2 = attr.float_val;
  }
  
  return result;
}

GeisAdapter::GeisRotateData * GeisAdapter::ProcessRotateGesture (GeisSize count, GeisGestureAttr *attrs)
{
  GeisRotateData *result = (GeisRotateData*) g_malloc0 (sizeof (GeisRotateData));
  
  int i;
  for (i = 0; i < (int) count; i++)
  {
    GeisGestureAttr attr = attrs[i];
    if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_DEVICE_ID))
      result->device_id = attr.integer_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_TIMESTAMP))
      result->timestamp = attr.integer_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_FOCUS_X))
      result->focus_x = attr.integer_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_FOCUS_Y))
      result->focus_y = attr.integer_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_TOUCHES))
      result->touches = attr.integer_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_ANGLE))
      result->angle = attr.float_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_ANGLE_DELTA))
      result->angle_delta = attr.float_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_ANGULAR_VELOCITY))
      result->angle_velocity = attr.float_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_BOUNDINGBOX_X1))
      result->bound_x1 = attr.float_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_BOUNDINGBOX_Y1))
      result->bound_y1 = attr.float_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_BOUNDINGBOX_X2))
      result->bound_x2 = attr.float_val;
    else if (g_str_equal (attr.name, GEIS_GESTURE_ATTRIBUTE_BOUNDINGBOX_Y2))
      result->bound_y2 = attr.float_val;
  }
  
  return result;
}

static const char* s_gestures[] = {
  GEIS_GESTURE_TYPE_DRAG3, GEIS_GESTURE_TYPE_TAP3, GEIS_GESTURE_TYPE_ROTATE3, GEIS_GESTURE_TYPE_PINCH3,
  GEIS_GESTURE_TYPE_DRAG4, GEIS_GESTURE_TYPE_TAP4, GEIS_GESTURE_TYPE_ROTATE4, GEIS_GESTURE_TYPE_PINCH4,
  GEIS_GESTURE_TYPE_SYSTEM,
  NULL
};

void
GeisAdapter::RegisterRootInstance ()
{
  static GeisInputFuncs input_funcs = {
    &GeisAdapter::InputDeviceAdded,
    &GeisAdapter::InputDeviceChanged,
    &GeisAdapter::InputDeviceRemoved
  };

  static GeisGestureFuncs gesture_funcs = {
    &GeisAdapter::GestureAdded,
    &GeisAdapter::GestureRemoved,
    &GeisAdapter::GestureStart,
    &GeisAdapter::GestureUpdate,
    &GeisAdapter::GestureFinish
  };
  
  GeisStatus status = GEIS_UNKNOWN_ERROR;
  
  GeisXcbWinInfo xcb_win_info;
  xcb_win_info.display_name  = NULL,
  xcb_win_info.screenp       = NULL,
  xcb_win_info.window_id     = _screen->root ();
  
  GeisWinInfo win_info = {
    GEIS_XCB_FULL_WINDOW,
    &xcb_win_info
  };
  GeisInstance instance;

  status = geis_init(&win_info, &instance);
  if (status != GEIS_STATUS_SUCCESS)
  {
    fprintf(stderr, "error in geis_init\n");
    return;
  }

  status = geis_input_devices(instance, &input_funcs, this);
  if (status != GEIS_STATUS_SUCCESS)
  {
    fprintf(stderr, "error subscribing to input devices\n");
    return;;
  }

  status = geis_subscribe(instance,
                          GEIS_ALL_INPUT_DEVICES,
                          s_gestures,
                          &gesture_funcs,
                          this);
  if (status != GEIS_STATUS_SUCCESS)
  {
    fprintf(stderr, "error subscribing to gestures\n");
    return;
  }
  
  _root_instance = instance;
}
