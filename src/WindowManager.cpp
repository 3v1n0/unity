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
  
  bool IsWindowOnCurrentDesktop (guint32 xid)
  {
    return true;
  }
  
  bool IsWindowObscured (guint32 xid)
  {
    return false;
  }

  void Restore (guint32 xid)
  {
    g_debug ("%s", G_STRFUNC);
  }

  void Minimize (guint32 xid)
  {
    g_debug ("%s", G_STRFUNC);
  }

  void Close (guint32 xid)
  {
    g_debug ("%s", G_STRFUNC);
  }
  
 void Activate (guint32 xid)
  {
    g_debug ("%s", G_STRFUNC);
  }

 void Raise (guint32 xid)
  {
    g_debug ("%s", G_STRFUNC);
  }

 void Lower (guint32 xid)
  {
    g_debug ("%s", G_STRFUNC);
  }

  nux::Geometry GetWindowGeometry (guint xid)
  {
    return nux::Geometry (0, 0, 1, 1);
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

#define NET_WM_MOVERESIZE_MOVE 8


void
WindowManager::StartMove (guint32 xid, int x, int y)
{
  XEvent ev;
  Display* d = nux::GetThreadGLWindow()->GetX11Display();

  /* We first need to ungrab the pointer. FIXME: Evil */

    XUngrabPointer(d, CurrentTime);

  // --------------------------------------------------------------------------
  // FIXME: This is a workaround until the non-paired events issue is fixed in
  // nux
  XButtonEvent bev = {
    ButtonRelease,
    0,
    False,
    d,
    0,
    0,
    0,
    CurrentTime,
    x, y,
    x, y,
    0,
    Button1,
    True
  };
  XEvent *e = (XEvent*)&bev;
  nux::GetGraphicsThread()->ProcessForeignEvent (e, NULL);

  ev.xclient.type    = ClientMessage;
  ev.xclient.display = d;

  ev.xclient.serial     = 0;
  ev.xclient.send_event = true;

  ev.xclient.window     = xid;
  ev.xclient.message_type = m_MoveResizeAtom;
  ev.xclient.format     = 32;

  ev.xclient.data.l[0] = x;
  ev.xclient.data.l[1] = y;
  ev.xclient.data.l[2] = NET_WM_MOVERESIZE_MOVE;
  ev.xclient.data.l[3] = 1;
  ev.xclient.data.l[4] = 1;

  XSendEvent (d, DefaultRootWindow (d), FALSE,
                 SubstructureRedirectMask | SubstructureNotifyMask,
                 &ev);

  XSync (d, FALSE);
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
  MotifWmHints *data = NULL;
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
		                  &bytes_after, (guchar **)&data);
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
    hints = data;
	
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
