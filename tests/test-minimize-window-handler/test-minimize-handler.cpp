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

    unsigned int id () { return mXid; }
    bool         minimized ();

    void minimize ();
    void unminimize ();

    void makeTransientFor (X11Window *w);
    void setClientLeader (X11Window *w);
    void printTransients ();

    std::vector<unsigned int> transients ();

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
  if (mMinimizedHandler)
    mMinimizedHandler->unminimize ();

  if (mCreated)
    XDestroyWindow (mDpy, mXid);
}

bool
X11Window::minimized ()
{
  return mMinimizedHandler.get () != NULL;
}

void
X11Window::minimize ()
{
  if (!mMinimizedHandler)
  {
    printf ("Fake minimize window 0x%x\n", (unsigned int) mXid);
    mMinimizedHandler = compiz::MinimizedWindowHandler::Ptr (new compiz::MinimizedWindowHandler (mDpy, mXid));
    mMinimizedHandler->minimize ();
  }
}

void
X11Window::unminimize ()
{
  if (mMinimizedHandler)
  {
    printf ("Fake unminimize window 0x%x\n", (unsigned int) mXid);
    mMinimizedHandler->unminimize ();
    mMinimizedHandler.reset ();
  }
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
  std::cout << "test-minimize-handler [WINDOW]" << std::endl;
  std::cout << "prompt will be for op, where op is one of " << std::endl;
  std::cout << "[m] minimize, [u] unminimize [q] to quit" << std::endl;
}

int main (int argc, char **argv)
{
  Display                    *dpy;
  Window                     xid = 0;
  X11Window                  *window;
  X11Window                  *transient = NULL;
  X11Window                  *hasClientLeader = NULL;
  std::string                option = "";
  bool                       shapeExt;
  int                        shapeEvent;
  int                        shapeError;
  bool                       terminate = false;

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

  shapeExt = XShapeQueryExtension (dpy, &shapeEvent, &shapeError);

  if (!shapeExt)
  {
    std::cerr << "No shape extension .. setting test to passed" << std::endl;
    XCloseDisplay (dpy);
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

  std::cout << "[m]inimize [u]nminimize [q]uit?" << std::endl;

  do
  {
    struct pollfd pfd[2];

    pfd[0].fd = ConnectionNumber (dpy);
    pfd[0].events = POLLIN;
    pfd[0].revents = 0;
    pfd[1].fd = STDIN_FILENO;
    pfd[1].events = POLLIN;
    pfd[1].revents = 0;

    poll (pfd, 2, -1);

    while (XPending (dpy))
    {
      XEvent ev;
      XNextEvent (dpy, &ev);
    }

    if (pfd[1].revents == POLLIN)
    {
      char buf[2];

      int ret = read (STDIN_FILENO, buf, 1);

      if (ret != EAGAIN || ret != EWOULDBLOCK)
      {
        buf[1] = '\0';
        if (strncmp (buf, "m", 1) == 0)
        {
          window->minimize ();
          std::cout << "[m]inimize [u]nminimize [q]uit?" << std::endl;
        }
        else if (strncmp (buf, "u", 1) == 0)
        {
          window->unminimize ();
          std::cout << "[m]inimize [u]nminimize [q]uit?" << std::endl;
        }
        else if (strncmp (buf, "q", 1) == 0)
        {
          terminate = true;
          std::cout << "[m]inimize [u]nminimize [q]uit?" << std::endl;
        }
      }
    }

  } while (!terminate);

  delete window;

  XCloseDisplay (dpy);

  return 0;
}
