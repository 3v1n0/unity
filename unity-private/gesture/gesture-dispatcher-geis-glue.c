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
 * Authored by Neil Jagdish Patel <neil.patel@canonical.com>
 *             Chase Douglas <chase.douglas@canonical.com>
 *
 */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/X.h>

#include <glib.h>
#include <glib-object.h>

#include <geis/geis.h>
#include <geis/geisimpl.h>
#include <grail.h>

#include "../unity-private.h"

static void gesture_began_callback (void            *cookie,
                                    GeisGestureType  gesture_type,
                                    GeisGestureId    gesture_id,
                                    GeisSize         attr_count,
                                    GeisGestureAttr *attrs);

static void gesture_continued_callback (void            *cookie,
                                        GeisGestureType  gesture_type,
                                        GeisGestureId    gesture_id,
                                        GeisSize         attr_count,
                                        GeisGestureAttr *attrs);

static void gesture_ended_callback (void            *cookie,
                                    GeisGestureType  gesture_type,
                                    GeisGestureId    gesture_id,
                                    GeisSize         attr_count,
                                    GeisGestureAttr *attrs);


static gboolean io_callback (GIOChannel   *source,
                             GIOCondition  condition,
                             gpointer      data);

static GObject           *dispatcher    = NULL;
static UnityGestureEvent *dispatch_event = NULL;
static GeisInstance       instance;

static GeisGestureFuncs gesture_funcs = {
  NULL,
  NULL,
  gesture_began_callback,
  gesture_continued_callback,
  gesture_ended_callback
};

/*
 * Public Functions
 */
void
unity_gesture_geis_dispatcher_glue_init (GObject *object)
{
  GeisStatus status;
  GeisXcbWinInfo xcb_win_info = {
    .display_name = NULL,
    .screenp = NULL,
    .window_id = gdk_x11_get_default_root_xwindow (),
  };
  GeisWinInfo win_info = {
    GEIS_XCB_FULL_WINDOW,
    &xcb_win_info
  };
  
  status = geis_init (&win_info, &instance);
  if (status != GEIS_STATUS_SUCCESS)
    {
      g_warning ("Error in geis_init");
      return;
    }

  status = geis_configuration_supported (instance, GEIS_CONFIG_UNIX_FD);
  if (status != GEIS_STATUS_SUCCESS)
    {
      g_warning ("Geis does not support Unix fd");
      return;
    }

  int fd = -1;
  status = geis_configuration_get_value (instance, GEIS_CONFIG_UNIX_FD, &fd);
  if (status != GEIS_STATUS_SUCCESS)
    {
      g_warning ("Unable to retrieve Geis td");
      return;
    }

  status = geis_subscribe (instance,
                           GEIS_ALL_INPUT_DEVICES,
                           GEIS_ALL_GESTURES,
                           &gesture_funcs,
                           NULL);
  if (status != GEIS_STATUS_SUCCESS)
    {
      g_warning ("Unable to subscribe to gestures");
      return;
    }

  dispatcher = object;
  dispatch_event = unity_gesture_event_new ();
  dispatch_event->pan_event = unity_gesture_pan_event_new ();
  dispatch_event->pinch_event = unity_gesture_pinch_event_new ();
  dispatch_event->tap_event = unity_gesture_tap_event_new ();

  GIOChannel *iochannel = g_io_channel_unix_new (fd);
  g_io_add_watch_full (iochannel,
                       G_PRIORITY_HIGH,
                       G_IO_IN,
                       io_callback,
                       NULL,
                       NULL);
}

void
unity_gesture_geis_dispatcher_glue_finish ()
{
  geis_finish (instance);
}

/*
 * Private Functions
 */
static void
print_attr(GeisGestureAttr *attr)
{
  fprintf(stdout, "\tattr %s=", attr->name);
  switch (attr->type)
  {
    case GEIS_ATTR_TYPE_BOOLEAN:
      fprintf(stdout, "%s(bool)\n", attr->boolean_val ? "true" : "false");
      break;
    case GEIS_ATTR_TYPE_FLOAT:
      fprintf(stdout, "%f (float)\n", attr->float_val);
      break;
    case GEIS_ATTR_TYPE_INTEGER:
      fprintf(stdout, "%d (int)\n", (int)attr->integer_val);
      break;
    case GEIS_ATTR_TYPE_STRING:
      fprintf(stdout, "\"%s\" (string)\n", attr->string_val);
      break;
    default:
      fprintf(stdout, "<unknown>\n");
      break;
  }
}

static gboolean
gesture_to_event (GeisGestureType  gesture_type,
                  GeisGestureId    gesture_id,
                  GeisSize         attr_count,
                  GeisGestureAttr *attrs)
{
  switch (gesture_type)
    {
    case GRAIL_TYPE_EDRAG:
    case GRAIL_TYPE_MDRAG:
      dispatch_event->type = UNITY_GESTURE_TYPE_PAN;
      dispatch_event->fingers = gesture_type == GRAIL_TYPE_EDRAG ? 3 : 4;
      break;

    case GRAIL_TYPE_EPINCH:
    case GRAIL_TYPE_MPINCH:
      dispatch_event->type = UNITY_GESTURE_TYPE_PINCH;
      dispatch_event->fingers = gesture_type == GRAIL_TYPE_EPINCH ? 3 : 4;
      break;

    case GRAIL_TYPE_TAP3:
    case GRAIL_TYPE_TAP4:
      dispatch_event->type = UNITY_GESTURE_TYPE_TAP;
      dispatch_event->fingers = gesture_type == GRAIL_TYPE_TAP3 ? 3 : 4;
      break;

    default:
      return FALSE;
    }

  /* Load in basic info */
  if (attr_count >= 8)
    {
      UnityGestureEvent *ev = dispatch_event;
      ev->gesture_id = (gint32)gesture_id;
      ev->device_id = (gint32)attrs[0].integer_val;
      ev->timestamp = (gint32)attrs[1].integer_val;
      ev->root_window = (gint32)attrs[2].integer_val;
      ev->event_window = (gint32)attrs[3].integer_val;
      ev->child_window = (gint32)attrs[4].integer_val;
      ev->root_x = attrs[5].float_val;
      ev->root_y = attrs[6].float_val;
      /* attrs[7] is the gesture name */
    }

  if (dispatch_event->type == UNITY_GESTURE_TYPE_PAN &&
      attr_count >= 14)
    {
      UnityGesturePanEvent *ev = dispatch_event->pan_event;
      ev->delta_x = attrs[8].float_val;
      ev->delta_y = attrs[9].float_val;
      ev->velocity_x = attrs[10].float_val;
      ev->velocity_y = attrs[11].float_val;
      ev->x2 = attrs[12].float_val;
      ev->y2 = attrs[13].float_val;
    }
  else if (dispatch_event->type == UNITY_GESTURE_TYPE_PINCH &&
           attr_count >= 15)
    {
      UnityGesturePinchEvent *ev = dispatch_event->pinch_event;
      ev->radius_delta = attrs[8].float_val;
      ev->radius_velocity = attrs[9].float_val;
      ev->radius = attrs[10].float_val;
      ev->bounding_box_x1 = attrs[11].float_val;
      ev->bounding_box_y1 = attrs[12].float_val;
      ev->bounding_box_x2 = attrs[13].float_val;
      ev->bounding_box_y2 = attrs[14].float_val;
    }
  else if (dispatch_event->type == UNITY_GESTURE_TYPE_TAP &&
           attr_count >= 11)
    {
      UnityGestureTapEvent *ev = dispatch_event->tap_event;
      ev->duration = attrs[8].float_val;
      ev->x2 = attrs[9].float_val;
      ev->y2 = attrs[10].float_val;
    }
  return TRUE;
}

static void
gesture_began_callback (void            *cookie,
                        GeisGestureType  gesture_type,
                        GeisGestureId    gesture_id,
                        GeisSize         attr_count,
                        GeisGestureAttr *attrs)
{
  
  dispatch_event->type = UNITY_GESTURE_TYPE_NONE;
  dispatch_event->state = UNITY_GESTURE_STATE_BEGAN;

  if (gesture_to_event (gesture_type, gesture_id, attr_count, attrs) != TRUE)
    return;

  if (dispatch_event->type != UNITY_GESTURE_TYPE_NONE)
    {
      g_signal_emit_by_name (dispatcher,
                             "gesture",
                             dispatch_event);
    }
}

static void
gesture_continued_callback (void            *cookie,
                            GeisGestureType  gesture_type,
                            GeisGestureId    gesture_id,
                            GeisSize         attr_count,
                            GeisGestureAttr *attrs)
{
  dispatch_event->type = UNITY_GESTURE_TYPE_NONE;
  dispatch_event->state = UNITY_GESTURE_STATE_CONTINUED;

  if (gesture_to_event (gesture_type, gesture_id, attr_count, attrs) != TRUE)
    return;

  if (dispatch_event->type != UNITY_GESTURE_TYPE_NONE)
    {
      g_signal_emit_by_name (dispatcher,
                             "gesture",
                             dispatch_event);
    }
}

static void
gesture_ended_callback (void            *cookie,
                        GeisGestureType  gesture_type,
                        GeisGestureId    gesture_id,
                        GeisSize         attr_count,
                        GeisGestureAttr *attrs)
{
  dispatch_event->type = UNITY_GESTURE_TYPE_NONE;
  dispatch_event->state = UNITY_GESTURE_STATE_ENDED;

  if (gesture_to_event (gesture_type, gesture_id, attr_count, attrs) != TRUE)
    return;

  if (dispatch_event->type != UNITY_GESTURE_TYPE_NONE)
    {
      g_signal_emit_by_name (dispatcher,
                             "gesture",
                             dispatch_event);
    } 
}

static gboolean
io_callback (GIOChannel   *source,
             GIOCondition  condition,
             gpointer      data)
{
  geis_event_dispatch (instance);

  return TRUE;
}
