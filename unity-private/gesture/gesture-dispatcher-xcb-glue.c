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

#include "gesture.h"

#define XCB_DISPATCHER_TIMEOUT 500

typedef struct
{
  GSource source;

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
unity_gesture_xcb_dispatcher_glue_init ()
{
  g_debug ("%s", G_STRFUNC);
  xcb_connection_t *connection;
  XCBSource        *source;
  uint16_t          mask_len = 1;
  uint32_t         *mask;

  connection = xcb_connect (NULL, NULL);

  mask = malloc(sizeof(uint32_t) * mask_len);
  *mask = 0xeeee;
  if (set_mask (connection, 0, 0, mask_len, mask))
    {
      g_warning ("Unable to initialize gesture dispatcher xcb");
      return;
    }

  source = (XCBSource*)g_source_new (&XCBFuncs, sizeof(XCBSource));
  g_source_attach (&source->source, NULL);
}

void
unity_gesture_xcb_dispatcher_glue_finish ()
{
  g_debug ("%s", G_STRFUNC);
}

void
unity_gesture_xcb_dispatcher_glue_main_iteration ()
{
  //g_debug ("%s", G_STRFUNC);
}

static gboolean
source_prepare (GSource     *source, gint *timeout)
{
  *timeout = XCB_DISPATCHER_TIMEOUT;
  return TRUE;
}

static gboolean
source_check   (GSource     *source)
{
  return TRUE;
}

static gboolean
source_dispatch(GSource     *source,
                GSourceFunc  callback,
                gpointer     user_data)
{
  unity_gesture_xcb_dispatcher_glue_main_iteration ();
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
