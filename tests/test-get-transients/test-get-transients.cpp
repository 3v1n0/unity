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
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <minimizedwindowhandler.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <unistd.h> 
#include <sstream>
#include <poll.h>
#include <cstring>

class X11Window
{
  public:

    X11Window (Display *, Window id = 0);
    ~X11Window ();

    void makeTransientFor (X11Window *w);
    void setClientLeader (X11Window *w);
    void printTransients ();

    std::vector<unsigned int> transients ();

    unsigned int id () { return mXid; }

  private:

    Window mXid;
    Display *mDpy;
    compiz::MinimizedWindowHandler::Ptr mMinimizedHandler;
    bool mCreated;
};

X11Window::X11Window (Display *dpy, Window id)
{
  if (id == 0)
  {
    XSetWindowAttributes attrib;
    XEvent		     e;

    attrib.background_pixel = 0x0;
    attrib.backing_pixel = 0x0;

    id = XCreateWindow (dpy, DefaultRootWindow (dpy), 0, 0, 100, 100, 0,
                        DefaultDepth (dpy, DefaultScreen (dpy)), InputOutput,
                        DefaultVisual (dpy, DefaultScreen (dpy)), CWBackingPixel | CWBackPixel, &attrib);

    XSelectInput (dpy, id, ExposureMask | StructureNotifyMask);
    XMapRaised (dpy, id);

    while (1)
    {
      XNextEvent (dpy, &e);
      bool exposed = false;

      switch (e.type)
      {
      case Expose:
        if (e.xexpose.window == id)
          exposed = true;
        break;
      default:
        break;
      }

      if (exposed)
        break;
    }

    XClearWindow (dpy, id);

    mCreated = true;
  }
  else
    mCreated = false;

  mXid = id;
  mDpy = dpy;
}

X11Window::~X11Window ()
{
  if (mCreated)
    XDestroyWindow (mDpy, mXid);
}

void
X11Window::makeTransientFor (X11Window *w)
{
  XSetTransientForHint (mDpy, mXid, w->id ());
  XSync (mDpy, false);
}

void
X11Window::setClientLeader (X11Window *w)
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
X11Window::transients ()
{
  if (!mMinimizedHandler)
    mMinimizedHandler = compiz::MinimizedWindowHandler::Ptr (new compiz::MinimizedWindowHandler (mDpy, mXid));

  return mMinimizedHandler->getTransients ();
}

void
X11Window::printTransients ()
{
  for (unsigned int &w : transients ())
    printf ("window id 0x%x\n", w);
}

void usage ()
{
  std::cout << "test-get-transients [WINDOW]" << std::endl;
}

int main (int argc, char **argv)
{
  Display                    *dpy;
  Window                     xid = 0;
  X11Window                  *window = NULL;
  X11Window                  *transient = NULL;
  X11Window                  *hasClientLeader = NULL;
  std::vector<unsigned int>  expectedTransients;

  if ((argc == 2 && std::string (argv[1]) == "--help") || argc > 3)
  {
    usage ();
    return 1;
  }

  dpy = XOpenDisplay (NULL);

  if (!dpy)
  {
    std::cerr << "Failed to open display ... setting test to passed" << std::endl;
    return 0;
  }

  if (argc > 1)
    std::stringstream (argv[1]) >> std::hex >> xid;

  window = new X11Window (dpy, xid);

  if (!xid)
  {
    transient = new X11Window (dpy, 0);
    hasClientLeader = new X11Window (dpy, 0);

    transient->makeTransientFor (window);
    window->setClientLeader (window);
    hasClientLeader->setClientLeader (window);
  }

  /* This assumes that compiz/X is going to process
   * our requests in 3 seconds. That's plenty of time */

  sleep (3);
  window->printTransients ();

  if (transient && hasClientLeader)
  {
    expectedTransients.push_back (transient->id ());
    expectedTransients.push_back (hasClientLeader->id ());

    if (expectedTransients != window->transients ())
    {
      delete transient;
      delete hasClientLeader;
      delete window;
      XCloseDisplay (dpy);

      std::cout << "FAIL: returned transients did not match expected transients" << std::endl;
      return 1;
    }
    else
      std::cout << "PASS: returned transients did match expected transients" << std::endl;
  }

  if (transient)
    delete transient;

  delete window;

  XCloseDisplay (dpy);

  return 0;
}
