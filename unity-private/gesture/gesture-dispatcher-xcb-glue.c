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
#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#include <X11/X.h>

#include <glib.h>
#include <glib-object.h>

#include <grail.h>

#include "gesture.h"

#include "../unity-private.h"

#define XCB_DISPATCHER_TIMEOUT 100

typedef struct
{
  GSource source;


  xcb_connection_t    *connection;
  xcb_generic_event_t *event;

  GObject *dispatcher;

  UnityGestureEvent    *gesture_event;

} XCBSource;

static gboolean source_prepare (GSource     *source, gint *timeout);
static gboolean source_check   (GSource     *source);
static gboolean source_dispatch(GSource     *source,
                                GSourceFunc  callback,
                                gpointer     user_data);
static void     source_finalize(GSource     *source);

static gint     set_mask (xcb_connection_t *connection,
                          xcb_window_t      window,
                          uint16_t          device_id,
                          uint16_t          mask_len,
                          uint32_t         *mask);

static GSourceFuncs XCBFuncs = {
  &source_prepare,
  &source_check,
  &source_dispatch,
  &source_finalize,
  0,
  0
};

void
unity_gesture_xcb_dispatcher_glue_init (GObject *object)
{
  g_debug ("%s", G_STRFUNC);
  xcb_connection_t *connection;
  XCBSource        *source;
  uint16_t          mask_len = 1;
  grail_mask_t     *mask;

  connection = xcb_connect (NULL, NULL);

  mask = calloc (1, sizeof(grail_mask_t) * mask_len);

  /* Fiddle */
  grail_mask_set (mask, GRAIL_TYPE_TAP3);
  grail_mask_set (mask, GRAIL_TYPE_TAP4);

  /* Gropes */
  grail_mask_set (mask, GRAIL_TYPE_EPINCH);
  grail_mask_set (mask, GRAIL_TYPE_MPINCH);

  /* Pots and */
  grail_mask_set (mask, GRAIL_TYPE_EDRAG);
  grail_mask_set (mask, GRAIL_TYPE_MDRAG);

  if (set_mask (connection, 0, 0, mask_len, (uint32_t*)mask))
    {
      g_warning ("Unable to initialize gesture dispatcher xcb");
      return;
    }

  source = (XCBSource*)g_source_new (&XCBFuncs, sizeof(XCBSource));
  source->connection = connection;
  source->dispatcher = object;
  source->gesture_event = unity_gesture_event_new ();
  source->gesture_event->pan_event = unity_gesture_pan_event_new ();
  source->gesture_event->pinch_event = unity_gesture_pinch_event_new ();
  source->gesture_event->tap_event = unity_gesture_tap_event_new ();

  g_source_attach (&source->source, NULL);
}

void
unity_gesture_xcb_dispatcher_glue_finish ()
{
  g_debug ("%s", G_STRFUNC);
}

void
unity_gesture_xcb_dispatcher_glue_main_iteration (XCBSource *source)
{
  g_debug ("%s", G_STRFUNC);
  const xcb_query_extension_reply_t *extension_info;
  xcb_connection_t *connection = source->connection;
  
	extension_info = xcb_get_extension_data(connection, &xcb_gesture_id);

  if (source->event != NULL)
    {
      xcb_generic_event_t *event;
      xcb_gesture_notify_event_t *gesture_event;
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

      gesture_event = (xcb_gesture_notify_event_t *)event;
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
        case GRAIL_TYPE_EDRAG:
        case GRAIL_TYPE_MDRAG:
          dispatch_event->type = UNITY_GESTURE_TYPE_PAN;
          dispatch_event->fingers = gesture_event->gesture_type == GRAIL_TYPE_EDRAG ? 3 : 4;
          break;
        case GRAIL_TYPE_EPINCH:
        case GRAIL_TYPE_MPINCH:
          dispatch_event->type = UNITY_GESTURE_TYPE_PINCH;
          dispatch_event->fingers = gesture_event->gesture_type == GRAIL_TYPE_EPINCH ? 3 : 4;
          break;
        case GRAIL_TYPE_TAP3:
        case GRAIL_TYPE_TAP4:
          dispatch_event->type = UNITY_GESTURE_TYPE_TAP;
          dispatch_event->fingers =
                        gesture_event->gesture_type == GRAIL_TYPE_TAP3 ? 3 : 4;
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
          dispatch_event->root_x = (gint32)gesture_event->focus_x;
          dispatch_event->root_y = (gint32)gesture_event->focus_y;
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

static gboolean
source_prepare (GSource     *source, gint *timeout)
{
  XCBSource *s = (XCBSource *)source;
  //const xcb_query_extension_reply_t *extention_info;

  *timeout = XCB_DISPATCHER_TIMEOUT;

  //extension_info = xcb_get_extension_data (s->connection, &xcb_gesture_id);

  s->event = xcb_poll_for_event (s->connection);
  
  return s->event == NULL ? FALSE : TRUE;
}

static gboolean
source_check   (GSource     *source)
{
  return ((XCBSource*)source)->event == NULL ? FALSE : TRUE;
}

static gboolean
source_dispatch(GSource     *source,
                GSourceFunc  callback,
                gpointer     user_data)
{
  unity_gesture_xcb_dispatcher_glue_main_iteration ((XCBSource*)source);
  return TRUE;
}

static void
source_finalize (GSource *source)
{

}

static int set_mask_on_window(xcb_connection_t *connection, xcb_window_t window,
			      uint16_t device_id, uint16_t mask_len,
			      uint32_t *mask) {
	xcb_generic_error_t *error;
	xcb_void_cookie_t select_cookie;
	xcb_gesture_get_selected_events_cookie_t events_cookie;
	xcb_gesture_get_selected_events_reply_t *events_reply;
	xcb_gesture_event_mask_iterator_t events_iterator;
	int masks_len_reply;
	int mask_len_reply;
	uint32_t *mask_reply;

	select_cookie = xcb_gesture_select_events_checked(connection, window,
							  device_id, mask_len,
							  mask);
	error = xcb_request_check(connection, select_cookie);
	if (error) {
		fprintf(stderr,
			"Error: Failed to select events on window 0x%x\n",
			window);
		return -1;
	}

	events_cookie = xcb_gesture_get_selected_events(connection, window);
	events_reply = xcb_gesture_get_selected_events_reply(connection,
							     events_cookie,
							     &error);
	if (!events_reply) {
		fprintf(stderr,
			"Error: Failed to receive selected events reply on "
			"window 0x%x\n", window);
		return -1;
	}

	masks_len_reply =
		xcb_gesture_get_selected_events_masks_length(events_reply);
	if (masks_len_reply != 1) {
		fprintf(stderr,
			"Error: Wrong selected masks length returned: %d\n",
			masks_len_reply);
		return -1;
	}

	events_iterator =
		xcb_gesture_get_selected_events_masks_iterator(events_reply);
	if (events_iterator.data->device_id != device_id) {
		fprintf(stderr,
			"Error: Incorrect device id returned by server: %d\n",
			events_iterator.data->device_id);
		return -1;
	}

	mask_len_reply =
		xcb_gesture_event_mask_mask_data_length(events_iterator.data);
	if (mask_len_reply != mask_len) {
		fprintf(stderr,
			"Error: Incorrect mask length returned by server: %d\n",
			mask_len_reply);
		return -1;
	}

	mask_reply = xcb_gesture_event_mask_mask_data(events_iterator.data);
	if (memcmp(mask, mask_reply, mask_len * 4) != 0) {
		fprintf(stderr, "Error: Incorrect mask returned by server\n");
		return -1;
	}

	free(events_reply);

	return 0;
}

static int set_mask_on_subwindows(xcb_connection_t *connection,
				  xcb_window_t window, uint16_t device_id,
				  uint16_t mask_len, uint32_t *mask) {
	xcb_generic_error_t *error;
	xcb_query_tree_cookie_t tree_cookie;
	xcb_query_tree_reply_t *tree_reply;
	xcb_window_t *children;
	int num_children;
	int i;

	if (set_mask_on_window(connection, window, device_id, mask_len, mask))
		return -1;

	tree_cookie = xcb_query_tree(connection, window);
	tree_reply = xcb_query_tree_reply(connection, tree_cookie, &error);
	if (!tree_reply) {
		fprintf(stderr, "Error: Failed to query tree for window 0x%x\n",
		window);
		return -1;
	}

	num_children = xcb_query_tree_children_length(tree_reply);
	if (num_children <= 0)
		return 0;

	children = xcb_query_tree_children(tree_reply);
	if (!children) {
		fprintf(stderr,
			"Error: Failed to retrieve children of window 0x%x\n",
			window);
		return -1;
	}

	for (i = 0; i < num_children; i++)
		if (set_mask_on_subwindows(connection, children[i], device_id,
					   mask_len, mask))
			return -1;
	
	free(tree_reply);

	return 0;
}

static int set_mask(xcb_connection_t *connection, xcb_window_t window,
		    uint16_t device_id, uint16_t mask_len, uint32_t *mask) {
	xcb_generic_error_t *error;
	xcb_gesture_query_version_cookie_t version_cookie;
	xcb_gesture_query_version_reply_t *version_reply;

	version_cookie = xcb_gesture_query_version(connection, 0, 5);
	version_reply = xcb_gesture_query_version_reply(connection,
							version_cookie, &error);
	if (!version_reply) {
		fprintf(stderr,
			"Error: Failed to receive gesture version reply\n");
		return -1;
	}

	if (version_reply->major_version != 0 &&
	    version_reply->minor_version != 5) {
		fprintf(stderr,
			"Error: Server supports unrecognized version: %d.%d\n",
			version_reply->major_version,
			version_reply->minor_version);
		return -1;
	}

	free(version_reply);

	if (window != 0) {
		if (set_mask_on_window(connection, window, device_id, mask_len,
				       mask))
			return -1;
	}
	else {
		const xcb_setup_t *setup;
		xcb_screen_iterator_t screen;

		setup = xcb_get_setup(connection);
		if (!setup) {
			fprintf(stderr, "Error: Failed to get xcb setup\n");
			return -1;
		}

		for (screen = xcb_setup_roots_iterator(setup); screen.rem;
		     xcb_screen_next(&screen))
			if (set_mask_on_subwindows(connection,
			    screen.data->root, device_id, mask_len, mask))
				return -1;
	}

	return 0;
}
