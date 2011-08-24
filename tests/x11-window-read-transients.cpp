/*
 * Copyright (C) 2011 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authored By:
 * Sam Spilsbury <sam.spilsbury@canonical.com>
 */

#include "x11-window-read-transients.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

X11WindowReadTransients::X11WindowReadTransients (Display *d, Window w) :
    X11Window::X11Window (d, w)
{
}

X11WindowReadTransients::~X11WindowReadTransients ()
{
}

void
X11WindowReadTransients::makeTransientFor (X11WindowReadTransients *w)
{
  XSetTransientForHint (mDpy, mXid, w->id ());
  XSync (mDpy, false);
}

void
X11WindowReadTransients::setClientLeader (X11WindowReadTransients *w)
{
  Atom wmClientLeader = XInternAtom (mDpy, "WM_CLIENT_LEADER", 0);
  Atom netWmWindowType = XInternAtom (mDpy, "_NET_WM_WINDOW_TYPE", 0);
  Atom netWmWindowTypeDialog = XInternAtom (mDpy, "_NET_WM_WINDOW_TYPE_DIALOG", 0);

  Window cl = w->id ();

  XChangeProperty (mDpy, mXid, wmClientLeader, XA_WINDOW, 32,
                   PropModeReplace, (unsigned char *) &cl, 1);
  XChangeProperty (mDpy, mXid, netWmWindowType, XA_ATOM, 32,
                   PropModeAppend, (const unsigned char *) &netWmWindowTypeDialog, 1);

  XSync (mDpy, false);
}

std::vector<unsigned int>
X11WindowReadTransients::transients ()
{
  compiz::X11TransientForReader *reader = new compiz::X11TransientForReader (mDpy, mXid);
  std::vector<unsigned int> transients = reader->getTransients ();

  delete reader;
  return transients;
}

void
X11WindowReadTransients::printTransients ()
{
  for (unsigned int &w : transients ())
    printf ("window id 0x%x\n", w);
}
