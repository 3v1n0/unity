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

static void gesture_callback (void            *cookie,
                              GeisGestureType  gesture_type,
                              GeisGestureId    gesture_id,
                              GeisSize         attr_count,
                              GeisGestureAttr *attrs);
static gboolean io_callback (GIOChannel   *source,
                             GIOCondition  condition,
                             gpointer      data);

static GObject           *dispatcher    = NULL;
static UnityGestureEvent *gesture_event = NULL;
static GeisInstance       instance;

static GeisGestureFuncs gesture_funcs = {
  gesture_callback,
  gesture_callback,
  gesture_callback,
  gesture_callback,
  gesture_callback
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
  gesture_event = unity_gesture_event_new ();
  gesture_event->pan_event = unity_gesture_pan_event_new ();
  gesture_event->pinch_event = unity_gesture_pinch_event_new ();
  gesture_event->tap_event = unity_gesture_tap_event_new ();

  GIOChannel *iochannel = g_io_channel_unix_new (fd);
  g_io_add_watch (iochannel,
                  G_IO_IN,
                  io_callback,
                  NULL);
}

void
unity_gesture_geis_dispatcher_glue_finish ()
{
  g_debug ("%s", G_STRFUNC);
}

/*
 * Private Functions
 */

static void
gesture_callback (void            *cookie,
                  GeisGestureType  gesture_type,
                  GeisGestureId    gesture_id,
                  GeisSize         attr_count,
                  GeisGestureAttr *attrs)
{
  g_debug ("Event");
}

static gboolean
io_callback (GIOChannel   *source,
             GIOCondition  condition,
             gpointer      data)
{
  geis_event_dispatch (instance);

  return TRUE;
}

#if 0
void
unity_gesture_geis_dispatcher_glue_main_iteration (XCBSource *source)
{
  g_debug ("%s", G_STRFUNC);
  const geis_query_extension_reply_t *extension_info;
  geis_connection_t *connection = source->connection;
  
	extension_info = geis_get_extension_data(connection, &geis_gesture_id);

  if (source->event != NULL)
    {
      geis_generic_event_t *event;
      geis_gesture_notify_event_t *gesture_event;
      float *properties;
      int i;
      UnityGestureEvent    *dispatch_event = source->gesture_event;

      dispatch_event->type = UNITY_GESTURE_TYPE_NONE;

      event = source->event;
      if (!event)
        { 
          fprintf (stderr, "Warning: I/O error while waiting for event\n");
          return;
        }

      if (event->response_type != GenericEvent)
        {
          fprintf (stderr, "Warning: Received non-generic event type: "
                   "%d\n", event->response_type);
          return;
        }

      gesture_event = (geis_gesture_notify_event_t *)event;
      properties = (float *)(gesture_event + 1);
      
      if (gesture_event->extension != extension_info->major_opcode)
        {
          fprintf (stderr, "Warning: Received non-gesture extension "
                   "event: %d\n", gesture_event->extension);
          return;
        }

      if (gesture_event->event_type != XCB_GESTURE_NOTIFY)
        {
          fprintf (stderr, "Warning: Received unrecognized gesture event "
                   "type: %d\n", gesture_event->event_type);
          return;
        }

      switch (gesture_event->gesture_type)
        {
        case GRAIL_TYPE_SWIPE:
        case GRAIL_TYPE_BRUSH:
          dispatch_event->type = UNITY_GESTURE_TYPE_PAN;
          dispatch_event->fingers = gesture_event->gesture_type == GRAIL_TYPE_SWIPE ? 3 : 4;
          break;
        case GRAIL_TYPE_SCALE:
        case GRAIL_TYPE_PICK:
          dispatch_event->type = UNITY_GESTURE_TYPE_PINCH;
          dispatch_event->fingers = gesture_event->gesture_type == GRAIL_TYPE_SCALE ? 3 : 4;
          dispatch_event->pinch_event->delta = properties[0];
          break;
        case GRAIL_TYPE_TAP3:
        case GRAIL_TYPE_TAP4:
          dispatch_event->type = UNITY_GESTURE_TYPE_TAP;
          dispatch_event->fingers =
                        gesture_event->gesture_type == GRAIL_TYPE_TAP3 ? 3 : 4;
          dispatch_event->tap_event->duration = properties[0];
          break;
        default:
          g_warning ("Unknown Gesture Event Type %d\n",
                     gesture_event->gesture_type);
          break;
        }

      if (dispatch_event->type != UNITY_GESTURE_TYPE_NONE)
        {
          dispatch_event->device_id = gesture_event->device_id;
          dispatch_event->gesture_id = gesture_event->gesture_id;
          dispatch_event->timestamp = gesture_event->time;
          dispatch_event->state = gesture_event->status;
          dispatch_event->x = (gint32)gesture_event->focus_x;
          dispatch_event->y = (gint32)gesture_event->focus_y;
          dispatch_event->root_window = gesture_event->root;
          dispatch_event->event_window = gesture_event->event;
          dispatch_event->child_window = gesture_event->child;

          g_signal_emit_by_name (source->dispatcher,
                                 "gesture",
                                 dispatch_event);
       }

      /*
      printf("\tDevice ID:\t%hu\n", gesture_event->device_id);
      printf("\tTimestamp:\t%u\n", gesture_event->time);

      printf("\tRoot Window:\t0x%x\n: (root window)\n",
             gesture_event->root);

      printf("\tEvent Window:\t0x%x\n: ", gesture_event->event);
      printf("\tChild Window:\t0x%x\n: ", gesture_event->child);
      printf("\tFocus X:\t%f\n", gesture_event->focus_x);
      printf("\tFocus Y:\t%f\n", gesture_event->focus_y);
      printf("\tStatus:\t\t%hu\n", gesture_event->status);
      */
      printf("\tNum Props:\t%hu\n", gesture_event->num_props);
     
    
      for (i = 0; i < gesture_event->num_props; i++) {
        printf("\t\tProperty %u:\t%f\n", i, properties[i]);
      }
    } 
}
#endif
