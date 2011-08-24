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

#include "minimizedwindowhandler.h"

namespace compiz
{
class PrivateMinimizedWindowHandler
{
public:

  PrivateMinimizedWindowHandler () {};

  Display      *mDpy;
  unsigned int mXid;

  WindowInputRemover *mRemover;
};
}

void
compiz::MinimizedWindowHandler::setVisibility (bool visible, Window shapeWin)
{
  if (!visible && !priv->mRemover)
  {
    priv->mRemover = new compiz::WindowInputRemover (priv->mDpy, shapeWin);
    if (!priv->mRemover)
	    return;

    if (priv->mRemover->save ())
	    priv->mRemover->remove ();
  }
  else if (visible && priv->mRemover)
  {
    priv->mRemover->restore ();

    delete priv->mRemover;
    priv->mRemover = NULL;
  }
}

std::vector<unsigned int>
compiz::MinimizedWindowHandler::removeState ()
{
  std::vector <unsigned int> atoms;

  atoms.push_back (XInternAtom (priv->mDpy, "_NET_WM_STATE_SKIP_TASKBAR", 0));
  atoms.push_back (XInternAtom (priv->mDpy, "_NET_WM_STATE_SKIP_PAGER", 0));

  return atoms;
}

std::vector<unsigned int>
compiz::MinimizedWindowHandler::getTransients ()
{
  unsigned long nItems, nLeft;
  int           actualFormat;
  Atom          actualType;
  Atom          wmClientList;
  unsigned char *prop;
  std::vector<unsigned int>   transients;
  std::vector<Window>   clientList;

  wmClientList = XInternAtom (priv->mDpy, "_NET_CLIENT_LIST", 0);

  if (XGetWindowProperty (priv->mDpy, priv->mXid, wmClientList, 0L, 512L, false,
                          XA_WINDOW, &actualType, &actualFormat, &nItems, &nLeft,
                          &prop))
  {
    if (actualType == XA_WINDOW && actualFormat == 32 && nItems && !nLeft)
    {
	    Window *data = (Window *) prop;

	    while (nItems--)
        clientList.push_back (*data++);
    }

    XFree (prop);
  }

  /* Now check all the windows in this client list
     * for the transient state (note that this won't
     * work for override redirect windows since there's
     * no concept of a client list for override redirect
     * windows, but it doesn't matter anyways in this
     * [external] case) */

  for (Window &w : clientList)
  {
    X11TransientForReader *reader = new X11TransientForReader (priv->mDpy, w);

    if (reader->isTransientFor (priv->mXid) ||
        reader->isGroupTransientFor (priv->mXid))
	    transients.push_back (w);

    delete reader;
  }

  return transients;
}

void
compiz::MinimizedWindowHandler::minimize ()
{
  Atom          wmState = XInternAtom (priv->mDpy, "WM_STATE", 0);
  unsigned long data[2];
  Window        root = DefaultRootWindow (priv->mDpy), parent = priv->mXid, lastParent = priv->mXid;
  Window        *children;
  unsigned int  nchildren;

  std::vector<unsigned int> transients = getTransients ();

  for (unsigned int &w : transients)
  {
    compiz::MinimizedWindowHandler *mw  = new compiz::MinimizedWindowHandler (priv->mDpy, w);

    mw->minimize ();

    delete mw;
  }

  do
  {
    if (XQueryTree (priv->mDpy, parent, &root, &parent, &children, &nchildren))
    {
      if (root != parent)
        lastParent = parent;
      XFree (children);
    }
    else
      root = parent;
  } while (root != parent);

  setVisibility (false, lastParent);

  data[0] = IconicState;
  data[1] = None;

  XChangeProperty (priv->mDpy, priv->mXid, wmState, wmState,
                   32, PropModeReplace, (unsigned char *) data, 2);
}

void
compiz::MinimizedWindowHandler::unminimize ()
{
  Atom          wmState = XInternAtom (priv->mDpy, "WM_STATE", 0);
  unsigned long data[2];
  Window        root = DefaultRootWindow (priv->mDpy), parent = priv->mXid, lastParent = priv->mXid;
  Window        *children;
  unsigned int  nchildren;

  std::vector<unsigned int> transients = getTransients ();

  for (unsigned int &w : transients)
  {
    compiz::MinimizedWindowHandler *mw  = new compiz::MinimizedWindowHandler (priv->mDpy, w);

    mw->unminimize ();

    delete mw;
  }

  do
  {
    if (XQueryTree (priv->mDpy, parent, &root, &parent, &children, &nchildren))
    {
      if (root != parent)
        lastParent = parent;
      XFree (children);
    }
    else
      root = parent;
  } while (root != parent);

  setVisibility (true, lastParent);

  data[0] = NormalState;
  data[1] = None;

  XChangeProperty (priv->mDpy, priv->mXid, wmState, wmState,
                   32, PropModeReplace, (unsigned char *) data, 2);
}

compiz::MinimizedWindowHandler::MinimizedWindowHandler (Display *dpy, unsigned int xid)
{
  priv = new PrivateMinimizedWindowHandler;

  priv->mDpy = dpy;
  priv->mXid = xid;
  priv->mRemover = NULL;
}

compiz::MinimizedWindowHandler::~MinimizedWindowHandler ()
{
  delete priv;
}
