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

#include <memory>
#include <minimizedwindowhandler.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <unistd.h> 
#include <sstream>
#include <poll.h>
#include <cstring>

#include <x11-window-read-transients.h>

class X11WindowFakeMinimizable :
  public X11WindowReadTransients,
  public compiz::WindowInputRemoverLockAcquireInterface
{
  public:

    X11WindowFakeMinimizable (Display *, Window id = 0);
    ~X11WindowFakeMinimizable ();

    unsigned int id () { return mXid; }
    bool         minimized ();

    void minimize ();
    void unminimize ();

  private:

    compiz::WindowInputRemoverLock::Ptr GetInputRemover ();

    compiz::WindowInputRemoverLock::Weak input_remover_;
    compiz::MinimizedWindowHandler::Ptr mMinimizedHandler;
};

compiz::WindowInputRemoverLock::Ptr
X11WindowFakeMinimizable::GetInputRemover ()
{
  if (!input_remover_.expired ())
    return input_remover_.lock ();

  compiz::WindowInputRemoverLock::Ptr ret (
    new compiz::WindowInputRemoverLock (
      new compiz::WindowInputRemover (mDpy, mXid, mXid)));
  input_remover_ = ret;
  return ret;
}

X11WindowFakeMinimizable::X11WindowFakeMinimizable (Display *d, Window id) :
  X11WindowReadTransients (d, id)
{
}

X11WindowFakeMinimizable::~X11WindowFakeMinimizable ()
{
}

bool
X11WindowFakeMinimizable::minimized ()
{
  return mMinimizedHandler.get () != NULL;
}

void
X11WindowFakeMinimizable::minimize ()
{
  if (!mMinimizedHandler)
  {
    printf ("Fake minimize window 0x%x\n", (unsigned int) mXid);
    mMinimizedHandler = compiz::MinimizedWindowHandler::Ptr (new compiz::MinimizedWindowHandler (mDpy, mXid, this));
    mMinimizedHandler->minimize ();
  }
}

void
X11WindowFakeMinimizable::unminimize ()
{
  if (mMinimizedHandler)
  {
    printf ("Fake unminimize window 0x%x\n", (unsigned int) mXid);
    mMinimizedHandler->unminimize ();
    mMinimizedHandler.reset ();
  }
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
  std::unique_ptr <X11WindowFakeMinimizable> window;
  std::unique_ptr <X11WindowFakeMinimizable> transient;
  std::unique_ptr <X11WindowFakeMinimizable> hasClientLeader;
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

  window.reset (new X11WindowFakeMinimizable (dpy, xid));

  if (!xid)
  {
    transient.reset (new X11WindowFakeMinimizable (dpy, 0));
    hasClientLeader.reset (new X11WindowFakeMinimizable (dpy, 0));

    transient->makeTransientFor (window.get ());
    window->setClientLeader (window.get ());
    hasClientLeader->setClientLeader (window.get ());
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

  XCloseDisplay (dpy);

  return 0;
}
