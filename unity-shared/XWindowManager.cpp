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
#include <NuxCore/Logger.h>
#include <core/core.h>
#include <core/atoms.h>
#include <gdk/gdkx.h>
#include "XWindowManager.h"

namespace unity
{
namespace
{
DECLARE_LOGGER(logger, "unity.windowmanager.x");

const long NET_WM_MOVERESIZE_MOVE = 8;

namespace atom
{
Atom _NET_WM_VISIBLE_NAME = 0;
Atom XA_COMPOUND_TEXT = 0;
}
}

XWindowManager::XWindowManager()
{
  atom::_NET_WM_VISIBLE_NAME = XInternAtom(screen->dpy(), "_NET_WM_VISIBLE_NAME", False);
  atom::XA_COMPOUND_TEXT = XInternAtom(screen->dpy(), "COMPOUND_TEXT", False);
}

std::string XWindowManager::GetStringProperty(Window window_id, Atom atom) const
{
  Atom type;
  int result, format;
  unsigned long n_items, bytes_after;
  char *val = nullptr;

  result = XGetWindowProperty(screen->dpy(), window_id, atom, 0L, 65536, False,
                              AnyPropertyType, &type, &format, &n_items,
                              &bytes_after, reinterpret_cast<unsigned char **>(&val));

  if (result != Success)
  {
    LOG_DEBUG(logger) << "Impossible to get the property " << gdk_x11_get_xatom_name(atom)
                      << " for window " << window_id;
    return std::string();
  }

  if (!val || n_items == 0)
  {
    LOG_DEBUG(logger) << "Impossible to get the property " << gdk_x11_get_xatom_name(atom)
                      << " for window " << window_id << ": empty value";
    return std::string();
  }

  std::unique_ptr<char[], int(*)(void*)> string(val, XFree);

  if (format != 8)
  {
    LOG_ERROR(logger) << "Impossible to get the property " << gdk_x11_get_xatom_name(atom)
                      << " for window " << window_id << ": invalid format or empty value";
    return std::string();
  }

  if (type != XA_STRING && type != atom::XA_COMPOUND_TEXT && type != Atoms::utf8String)
  {
    LOG_ERROR(logger) << "Impossible to get the property " << gdk_x11_get_xatom_name(atom)
                      << " for window " << window_id << ": invalid string type";
    return std::string();
  }

  if (type == atom::XA_COMPOUND_TEXT)
  {
    // In case we have compound text, we need to convert it to utf-8
    XTextProperty text_property;
    text_property.value = (unsigned char *)val;
    text_property.encoding = type;
    text_property.format = format;
    text_property.nitems = n_items;

    char **list = nullptr;
    int count = 0;
    result = XmbTextPropertyToTextList(screen->dpy(), &text_property, &list, &count);

    if (result != Success || count == 0)
    {
      LOG_WARN(logger) << "Impossible to get the property " << gdk_x11_get_xatom_name(atom)
                       << "for window " << window_id << " properly: impossible to convert to current locale";
      return std::string(val, n_items);
    }

    std::unique_ptr<char*[], void(*)(char**)> list_ptr(list, XFreeStringList);

    if (count != 1)
    {
      LOG_WARN(logger) << "Impossible to get the property " << gdk_x11_get_xatom_name(atom)
                       << "for window " << window_id << " properly: invalid number of parsed values";
    }

    return list_ptr[0];
  }

  return std::string(val, n_items);
}

std::vector<long> XWindowManager::GetCardinalProperty(Window window_id, Atom atom) const
{
  Atom type;
  int result, format;
  unsigned long n_items, bytes_after;
  long *buf = nullptr;

  result = XGetWindowProperty(screen->dpy(), window_id, atom, 0L, 65536, False,
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
  std::string name = GetStringProperty(window_id, atom::_NET_WM_VISIBLE_NAME);

  if (!name.empty())
    return name;

  name = GetStringProperty(window_id, Atoms::wmName);

  if (!name.empty())
    return name;

  return GetStringProperty(window_id, XA_WM_NAME);
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
