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

  std::list <MinimizedWindowHandler::Ptr> mTransients;

  WindowInputRemover *mRemover;
};
}

bool
compiz::MinimizedWindowHandler::contains (boost::shared_ptr <MinimizedWindowHandler> mw)
{
  for (MinimizedWindowHandler::Ptr h : priv->mTransients)
  {
    if (h->priv->mXid == mw->priv->mXid)
      return true;
  }

  return false;
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
compiz::MinimizedWindowHandler::getTransients ()
{
  std::vector<unsigned int> transients;
  compiz::X11TransientForReader *reader = new compiz::X11TransientForReader (priv->mDpy, priv->mXid);

  transients = reader->getTransients ();

  delete reader;

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
  compiz::MinimizedWindowHandler::Ptr holder = compiz::MinimizedWindowHandler::Ptr (new compiz::MinimizedWindowHandler (priv->mDpy, 0));
  auto          predicate_this = boost::bind (&compiz::MinimizedWindowHandler::contains, this, _1);
  auto          predicate_holder = !boost::bind (&compiz::MinimizedWindowHandler::contains, holder.get (), _1);

  std::vector<unsigned int> transients = getTransients ();

  for (unsigned int &w : transients)
  {
    compiz::MinimizedWindowHandler::Ptr p = compiz::MinimizedWindowHandler::Ptr (new compiz::MinimizedWindowHandler (priv->mDpy, w));
    holder->priv->mTransients.push_back (p);
  }

  priv->mTransients.remove_if (predicate_holder);
  holder->priv->mTransients.remove_if (predicate_this);

  for (MinimizedWindowHandler::Ptr &mw : holder->priv->mTransients)
    priv->mTransients.push_back (mw);

  for (MinimizedWindowHandler::Ptr &mw : priv->mTransients)
    mw->minimize ();

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
  compiz::MinimizedWindowHandler::Ptr holder = compiz::MinimizedWindowHandler::Ptr (new compiz::MinimizedWindowHandler (priv->mDpy, 0));
  auto          predicate_this = boost::bind (&compiz::MinimizedWindowHandler::contains, this, _1);
  auto          predicate_holder = !boost::bind (&compiz::MinimizedWindowHandler::contains, holder.get (), _1);

  std::vector<unsigned int> transients = getTransients ();

  for (unsigned int &w : transients)
  {
    compiz::MinimizedWindowHandler::Ptr p = compiz::MinimizedWindowHandler::Ptr (new compiz::MinimizedWindowHandler (priv->mDpy, w));
    holder->priv->mTransients.push_back (p);
  }

  priv->mTransients.remove_if (predicate_holder);
  holder->priv->mTransients.remove_if (predicate_this);

  for (MinimizedWindowHandler::Ptr &mw : holder->priv->mTransients)
    priv->mTransients.push_back (mw);

  for (MinimizedWindowHandler::Ptr &mw : priv->mTransients)
    mw->unminimize ();

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
