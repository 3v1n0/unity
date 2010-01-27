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

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

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
