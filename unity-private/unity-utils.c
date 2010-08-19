/*
 * Copyright (C) 2009 Canonical Ltd
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

#include <config.h>

#include <string.h>
#include <stdio.h>

#include <dbus/dbus-glib.h>

#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>

#include <libindicator/indicator-object.h>

#include <gio/gio.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

gboolean utils_compare_images (const gchar *img1_path,
                               const gchar *img2_path);

/* Taken from Maximus (lp:maximus) */

/* <TAKEN FROM GDK> */
typedef struct {
    unsigned long flags;
    unsigned long functions;
    unsigned long decorations;
    long input_mode;
    unsigned long status;
} MotifWmHints, MwmHints;

#define MWM_HINTS_FUNCTIONS     (1L << 0)
#define MWM_HINTS_DECORATIONS   (1L << 1)
#define _XA_MOTIF_WM_HINTS		"_MOTIF_WM_HINTS"

gboolean
utils_window_is_decorated (Window xid)
{
  GdkDisplay *display = gdk_display_get_default();
  Atom hints_atom = None;
  guchar *data = NULL;
  MotifWmHints *hints = NULL;
  Atom type = None;
  gint format;
  gulong nitems;
  gulong bytes_after;
  gboolean retval;
 
  hints_atom = gdk_x11_get_xatom_by_name_for_display (display, 
                                                      _XA_MOTIF_WM_HINTS);

  XGetWindowProperty (GDK_DISPLAY_XDISPLAY (display), 
                      xid,
		                  hints_atom, 0, sizeof (MotifWmHints)/sizeof (long),
		                  False, AnyPropertyType, &type, &format, &nitems,
		                  &bytes_after, &data);
  
  if (type == None || !data)
    {
      return TRUE;
    }
  
  hints = (MotifWmHints *)data; 
  
  retval = hints->decorations;
 
  if (data)
    XFree (data);

  return retval == 1;
}

static void
gdk_window_set_mwm_hints (Window        xid,
                          MotifWmHints *new_hints)
{
  GdkDisplay *display = gdk_display_get_default();
  Atom hints_atom = None;
  guchar *data = NULL;
  MotifWmHints *hints = NULL;
  Atom type = None;
  gint format;
  gulong nitems;
  gulong bytes_after;

  g_return_if_fail (GDK_IS_DISPLAY (display));

  g_debug ("gdk_window_set_mwm_hints: %lu %lu\n", xid, new_hints->decorations);
  
  hints_atom = gdk_x11_get_xatom_by_name_for_display (display, 
                                                      _XA_MOTIF_WM_HINTS);

  XGetWindowProperty (GDK_DISPLAY_XDISPLAY (display), 
                      xid,
		                  hints_atom, 0, sizeof (MotifWmHints)/sizeof (long),
		                  False, AnyPropertyType, &type, &format, &nitems,
		                  &bytes_after, &data);
  
  if (type != hints_atom || !data)
    hints = new_hints;
  else
  {
    hints = (MotifWmHints *)data;
	
    if (new_hints->flags & MWM_HINTS_FUNCTIONS)
    {
      hints->flags |= MWM_HINTS_FUNCTIONS;
      hints->functions = new_hints->functions;  
    }
    if (new_hints->flags & MWM_HINTS_DECORATIONS)
    {
      hints->flags |= MWM_HINTS_DECORATIONS;
      hints->decorations = new_hints->decorations;
    }
  }
  
  gdk_error_trap_push ();
  XChangeProperty (GDK_DISPLAY_XDISPLAY (display), 
                   xid,
                   hints_atom, hints_atom, 32, PropModeReplace,
                   (guchar *)hints, sizeof (MotifWmHints)/sizeof (long));
  gdk_flush ();
  if (gdk_error_trap_pop ())
    {
      g_debug ("ERROR:");
    }
  
  if (data)
    XFree (data);
}

void
utils_window_set_decorations (Window          xid,
			                        GdkWMDecoration decorations)
{
  MotifWmHints *hints;
    
  /* initialize to zero to avoid writing uninitialized data to socket */
  hints = g_slice_new0 (MotifWmHints);
  hints->flags = MWM_HINTS_DECORATIONS;
  hints->decorations = decorations;
 
  gdk_window_set_mwm_hints (xid, hints);

  g_slice_free (MotifWmHints, hints);
}

/* </ Taken from Maximus >*/

/* Taken from AWN (lp:awn) */

enum {
	STRUT_LEFT = 0,
	STRUT_RIGHT = 1,
	STRUT_TOP = 2,
	STRUT_BOTTOM = 3,
	STRUT_LEFT_START = 4,
	STRUT_LEFT_END = 5,
	STRUT_RIGHT_START = 6,
	STRUT_RIGHT_END = 7,
	STRUT_TOP_START = 8,
	STRUT_TOP_END = 9,
	STRUT_BOTTOM_START = 10,
	STRUT_BOTTOM_END = 11
};

//pulled out of xutils.c
static Atom net_wm_strut              = 0;
static Atom net_wm_strut_partial      = 0;

void
utils_set_strut (GtkWindow *gtk_window,
                 guint32    left_size,
                 guint32    left_start,
                 guint32    left_end,
                 guint32    top_size,
                 guint32    top_start,
                 guint32    top_end)
{
  Display   *display;
  Window     window;
  GdkWindow *gdk_window;
  gulong     struts [12] = { 0, };

  g_return_if_fail (GTK_IS_WINDOW (gtk_window));

  if (!left_size)
    return;

  gdk_window = gtk_widget_get_window (GTK_WIDGET (gtk_window));
  display = GDK_WINDOW_XDISPLAY (gdk_window);
  window  = GDK_WINDOW_XWINDOW (gdk_window);

  if (net_wm_strut == None)
    net_wm_strut = XInternAtom (display, "_NET_WM_STRUT", False);
  if (net_wm_strut_partial == None)
    net_wm_strut_partial = XInternAtom (display, "_NET_WM_STRUT_PARTIAL",False);

  struts [STRUT_LEFT] = left_size;
  struts [STRUT_LEFT_START] = left_start;
  struts [STRUT_LEFT_END] = left_end;

  struts [STRUT_TOP] = top_size;
  struts [STRUT_TOP_START] = top_start;
  struts [STRUT_TOP_END] = top_end;

  gdk_error_trap_push ();
  XChangeProperty (display, window, net_wm_strut,
                   XA_CARDINAL, 32, PropModeReplace,
                   (guchar *) &struts, 4);
  XChangeProperty (display, window, net_wm_strut_partial,
                   XA_CARDINAL, 32, PropModeReplace,
                   (guchar *) &struts, 12);
  gdk_error_trap_pop ();
}

void
utils_register_object_on_dbus (DBusGConnection *conn,
                          const gchar     *path,
                          GObject         *object)
{
  dbus_g_connection_register_g_object (conn, path, object);
}

Window
utils_get_stage_window (ClutterStage *stage)
{
  g_return_val_if_fail (CLUTTER_IS_STAGE (stage), 0);

  return clutter_x11_get_stage_window (stage);
}

gboolean
utils_save_snapshot (ClutterStage *stage,
                     const gchar  *filepath,
                     gint          x,
                     gint          y,
                     gint          width,
                     gint          height)
{
  guchar    *data;
  GdkPixbuf *pixbuf;
  GError    *error = NULL;

  g_return_val_if_fail (CLUTTER_IS_STAGE (stage), FALSE);
  g_return_val_if_fail (filepath != NULL, FALSE);

  /* Flush current events otherwise we get in trouble sometimes */
  while (gtk_events_pending ())
    gtk_main_iteration ();

  data = clutter_stage_read_pixels (stage, x, y, width, height);

  if (!data)
    {
      g_warning ("Unable to read stage pixels");
      return FALSE;
    }

  pixbuf = gdk_pixbuf_new_from_data (data,
                                     GDK_COLORSPACE_RGB,
                                     TRUE,
                                     8,
                                     width,
                                     height,
                                     width * 4,
                                     (GdkPixbufDestroyNotify)g_free,
                                     NULL);

  if (!gdk_pixbuf_save (pixbuf, filepath, "png", &error, NULL))
    {
      g_warning ("Unable to save snapshot: %s", error->message);
      g_error_free (error);
      g_object_unref (pixbuf);

      return FALSE;
    }

  g_object_unref (pixbuf);
  return TRUE;
}

gboolean
utils_compare_snapshot (ClutterStage  *stage,
                        const gchar   *filepath,
                        gint           x,
                        gint           y,
                        gint           width,
                        gint           height,
                        gboolean       expected)
{
  guchar    *data;
  GdkPixbuf *pixbuf;
  GError    *error = NULL;
  gchar     *tmppath;
  gboolean   similar;

  g_return_val_if_fail (CLUTTER_IS_STAGE (stage), FALSE);
  g_return_val_if_fail (filepath != NULL, FALSE);

  /* Flush current events otherwise we get in trouble sometimes */
  while (gtk_events_pending ())
    gtk_main_iteration ();

  /* Grab the snapshot of the region passed in */
  data = clutter_stage_read_pixels (stage, x, y, width, height);

  if (!data)
    {
      g_warning ("Unable to read stage pixels");
      return !expected;
    }

  /* Convert to pixbuf then save so we can ask for a comparison */
  pixbuf = gdk_pixbuf_new_from_data (data,
                                     GDK_COLORSPACE_RGB,
                                     TRUE,
                                     8,
                                     width,
                                     height,
                                     width * 4,
                                     (GdkPixbufDestroyNotify)g_free,
                                     NULL);

  tmppath = g_strdup_printf ("%s.failed.png", filepath);

  if (!gdk_pixbuf_save (pixbuf, tmppath, "png", &error, NULL))
    {
      g_warning ("Unable to save snapshot: %s", error->message);
      g_error_free (error);
      g_object_unref (pixbuf);
      g_free (tmppath);

      return !expected;
    }

  /* Now compare the images */
  similar = utils_compare_images (filepath, tmppath);
  if (similar == expected)
    {
      /* As the comparison was expected, clean up comparison file */
      GFile *f = g_file_new_for_path (tmppath);
      g_file_delete (f, NULL, NULL);
      g_object_unref (f);
    }

  g_object_unref (pixbuf);
  g_free (tmppath);

  return expected == similar;
}

/**
 * utils_compare_images:
 * @img1_path: full path to first image
 * @img2_path: full path to compare @img1_path too
 *
 * Returns: %TRUE if images are similar or %FALSE if not
 **/
gboolean
utils_compare_images (const gchar *img1_path,
                      const gchar *img2_path)
{
  gint    exit_status = 0;
  gchar  *command;
  GError *error = NULL;

  g_return_val_if_fail (img1_path != NULL, FALSE);
  g_return_val_if_fail (img2_path != NULL, FALSE);

  command = g_strdup_printf ("perceptualdiff %s %s", img1_path, img2_path);

  if (!g_spawn_command_line_sync (command, NULL, NULL, &exit_status, &error))
    {
      g_warning ("Unable to run image comparison for '%s' and '%s': %s",
                 img1_path,
                 img2_path,
                 error ? error->message : "unknown");
      if (error)
        g_error_free (error);

      exit_status = 0;
    }

  g_free (command);

  /* perceptualdiff exits '0' on failed comparison and '1' on successful */
  return exit_status != 0;
}

IndicatorObjectEntry *
indicator_object_entry_new ()
{
  return g_new0 (IndicatorObjectEntry, 1);
}

void
indicator_object_entry_free (IndicatorObjectEntry *entry)
{
  if (G_LIKELY (entry))
    g_free (entry);

  entry = NULL;
}


static void
on_volume_ejected (GVolume          *volume,
                   GAsyncResult     *res)
{
  g_volume_eject_with_operation_finish (volume, res, NULL);
}

void
utils_volume_eject (GVolume *volume)
{
  g_return_if_fail (G_IS_VOLUME (volume));

  g_volume_eject_with_operation (volume, 0, NULL, NULL,
                                 (GAsyncReadyCallback)on_volume_ejected, NULL);
}
