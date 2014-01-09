// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010-2012 Canonical Ltd
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

#include <Nux/Nux.h>
#include <core/core.h>
#include <core/atoms.h>
#include <gdk/gdkx.h>
#include "XWindowManager.h"

namespace unity
{
namespace
{
const long NET_WM_MOVERESIZE_MOVE = 8;
}

std::string XWindowManager::GetUtf8Property(Window window_id, Atom atom) const
{
  Atom type;
  int result, format;
  unsigned long n_items, bytes_after;
  char *val = nullptr;
  std::string retval;
  Display* dpy = nux::GetGraphicsDisplay()->GetX11Display();

  result = XGetWindowProperty(dpy, window_id, atom, 0L, 65536, False,
                              Atoms::utf8String, &type, &format, &n_items,
                              &bytes_after, reinterpret_cast<unsigned char **>(&val));

  if (result != Success)
    return retval;

  if (type == Atoms::utf8String && format == 8 && val && n_items > 0)
  {
    retval = std::string(val, n_items);
  }

  XFree(val);

  return retval;
}

std::string XWindowManager::GetTextProperty(Window window_id, Atom atom) const
{
  XTextProperty text;
  std::string retval;
  text.nitems = 0;
  Display* dpy = nux::GetGraphicsDisplay()->GetX11Display();

  if (XGetTextProperty(dpy, window_id, &text, atom))
  {
    if (text.value)
    {
      retval = std::string(reinterpret_cast<char*>(text.value), text.nitems);
      XFree(text.value);
    }
  }

  return retval;
}

std::vector<long> XWindowManager::GetCardinalProperty(Window window_id, Atom atom) const
{
  Atom type;
  int result, format;
  unsigned long n_items, bytes_after;
  long *buf = nullptr;
  Display* dpy = nux::GetGraphicsDisplay()->GetX11Display();

  result = XGetWindowProperty(dpy, window_id, atom, 0L, 65536, False,
                              XA_CARDINAL, &type, &format, &n_items, &bytes_after,
                              reinterpret_cast<unsigned char **>(&buf));

  std::unique_ptr<long[], int(*)(void*)> buffer(buf, XFree);

  if (result == Success && type == XA_CARDINAL && format == 32 && buffer && n_items > 0)
  {
    std::vector<long> values(n_items);

    for (unsigned i = 0; i < n_items; ++i)
      values[i] = buffer[i];

    return values;
  }

  return std::vector<long>();
}

std::string XWindowManager::GetWindowName(Window window_id) const
{
  std::string name;
  Atom visibleNameAtom;

  visibleNameAtom = gdk_x11_get_xatom_by_name("_NET_WM_VISIBLE_NAME");
  name = GetUtf8Property(window_id, visibleNameAtom);

  if (name.empty())
    name = GetUtf8Property(window_id, Atoms::wmName);

  if (name.empty())
    name = GetTextProperty(window_id, XA_WM_NAME);

  return name;
}

void XWindowManager::StartMove(Window window_id, int x, int y)
{
  if (x < 0 || y < 0)
    return;

  XEvent ev;
  Display* d = nux::GetGraphicsDisplay()->GetX11Display();

  /* We first need to ungrab the pointer. FIXME: Evil */

  XUngrabPointer(d, CurrentTime);

  // --------------------------------------------------------------------------
  // FIXME: This is a workaround until the non-paired events issue is fixed in
  // nux
  XButtonEvent bev =
  {
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
  XEvent* e = (XEvent*)&bev;
  nux::GetWindowThread()->ProcessForeignEvent(e, NULL);

  ev.xclient.type    = ClientMessage;
  ev.xclient.display = d;

  ev.xclient.serial     = 0;
  ev.xclient.send_event = true;

  ev.xclient.window     = window_id;
  ev.xclient.message_type = Atoms::wmMoveResize;
  ev.xclient.format     = 32;

  ev.xclient.data.l[0] = x; // x_root
  ev.xclient.data.l[1] = y; // y_root
  ev.xclient.data.l[2] = NET_WM_MOVERESIZE_MOVE; //direction
  ev.xclient.data.l[3] = 1; // button
  ev.xclient.data.l[4] = 2; // source

  XSendEvent(d, DefaultRootWindow(d), FALSE,
             SubstructureRedirectMask | SubstructureNotifyMask,
             &ev);

  XSync(d, FALSE);
}
} // unity namespace
