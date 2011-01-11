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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#include "WindowManager.h"

#include <gdk/gdkx.h>
#include <X11/Xatom.h>

#include <stdlib.h>

#define BORDER_FOR_MAXIMIZE 100

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

static void gdk_window_set_mwm_hints (Window        xid,
                                      MotifWmHints *new_hints);


static WindowManager *window_manager = NULL;

class WindowManagerDummy : public WindowManager
{
  bool IsWindowMaximized (guint32 xid)
  {
    return false;
  }

  bool IsWindowDecorated (guint32 xid)
  {
    return true;
  }

  void Restore (guint32 xid)
  {
    g_debug ("%s", G_STRFUNC);
  }

  void Minimize (guint32 xid)
  {
    g_debug ("%s", G_STRFUNC);
  }

  void Maximize (guint32 xid)
  {
    g_debug ("%s", G_STRFUNC);
  }

 void Close (guint32 xid)
  {
    g_debug ("%s", G_STRFUNC);
  }
};

WindowManager *
WindowManager::Default ()
{
  if (!window_manager)
    window_manager = new WindowManagerDummy ();

  return window_manager;
}

void
WindowManager::SetDefault (WindowManager *manager)
{
  window_manager = manager;
  window_manager->window_mapped.connect (sigc::mem_fun (manager, &WindowManager::MaximizeIfBigEnough));
}

void
WindowManager::Decorate (guint32 xid)
{
  MotifWmHints hints = { 0 };
    
  hints.flags = MWM_HINTS_DECORATIONS;
  hints.decorations = GDK_DECOR_ALL;
 
  gdk_window_set_mwm_hints (xid, &hints);
}

void
WindowManager::Undecorate (guint32 xid)
{
  MotifWmHints hints = { 0 };
    
  hints.flags = MWM_HINTS_DECORATIONS;
  hints.decorations = 0;
 
  gdk_window_set_mwm_hints (xid, &hints);
}

/*
 * Maximize the window if near fullscreen coordonates
*/
void WindowManager::MaximizeIfBigEnough (guint32 xid)
{
  int win_x, win_y, area_x, area_y;
  unsigned int   win_width, win_height, area_width, area_height, bw, depth;
  Atom           xa_ret_type;
  Window         root_window;
  int            ret_format;
  unsigned long  ret_nitems;
  unsigned long  ret_bytes_after;
  unsigned int  *nb_desktop;
  long          *workareas;

  Display *dpy = XOpenDisplay(NULL);
  root_window = RootWindow(dpy, 0);

  if (XGetWindowProperty(dpy, root_window,
                         XInternAtom(dpy, "_NET_NUMBER_OF_DESKTOPS", 0), 0,
                         1, False, XA_CARDINAL, &xa_ret_type, &ret_format,
                         &ret_nitems, &ret_bytes_after, (guchar **) &nb_desktop) != Success)
  {
    g_warning ("Cannot get _NET_NUMBER_OF_DESKTOPS property");
    return;
  }

  if (xa_ret_type != XA_CARDINAL
     || ret_nitems != 1
     || ret_format != 32) {
    g_warning ("Invalid type of _NET_NUMBER_OF_DESKTOPS property");
    return;
  }
  

  // get the NET_WORKAREA
  if (XGetWindowProperty(dpy, root_window,
                         XInternAtom(dpy, "_NET_WORKAREA", 0), 0,
                         ((*nb_desktop) * 4 * 4), False, XA_CARDINAL,
                         &xa_ret_type, &ret_format, &ret_nitems,
                         &ret_bytes_after, (unsigned char **) &workareas) != Success)
  {
    g_warning ("Cannot get _NET_WORKAREA property");
    return;
  }

  if (xa_ret_type != XA_CARDINAL
      || (workareas == NULL)
      || ret_format != 32) {
    g_warning ("Invalid type of _NET_WORKAREA property");
    return;
  }

  area_x = workareas[0];
  area_y = workareas[1];
  area_width = workareas[2];
  area_height = workareas[3];

  XGetGeometry (dpy,
                (Window) xid,
                &root_window,
                &win_x, &win_y, &win_width, &win_height, &bw, &depth);
  gdk_flush ();
  if (gdk_error_trap_pop ())
    {
      g_debug ("ERROR: Cannot get XWindow property");
      return;
    }

  // FIXME: integrate decorator size and MULTIMONITOR
  if ((abs(win_x - area_x) < BORDER_FOR_MAXIMIZE)
      && (abs(win_width - area_width) < BORDER_FOR_MAXIMIZE)
      && (abs(win_y - area_y) < BORDER_FOR_MAXIMIZE)
      && (abs(win_height - area_height) < BORDER_FOR_MAXIMIZE))
  {
    g_debug ("Window maximized automatically");
    Undecorate (xid);
    Maximize (xid);
  }

  if (nb_desktop != NULL)
    XFree (nb_desktop);
  if (workareas != NULL)
    XFree (workareas);

}

/*
 * Copied over C code
 */
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
  
  hints_atom = gdk_x11_get_xatom_by_name_for_display (display, 
                                                      _XA_MOTIF_WM_HINTS);

  gdk_error_trap_push ();
  XGetWindowProperty (GDK_DISPLAY_XDISPLAY (display), 
                      xid,
		                  hints_atom, 0, sizeof (MotifWmHints)/sizeof (long),
		                  False, AnyPropertyType, &type, &format, &nitems,
		                  &bytes_after, &data);
  gdk_flush ();
  if (gdk_error_trap_pop ())
    {
      g_debug ("ERROR: Cannot set decorations");
      return;
    }
    
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
      g_debug ("ERROR: Setting decorations");
    }
  
  if (data)
    XFree (data);
}
