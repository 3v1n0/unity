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

#ifndef _COMPIZ_MINIMIZEDWINDOWHANDLER_H
#define _COMPIZ_MINIMIZEDWINDOWHANDLER_H

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include "transientfor.h"
#include "inputremover.h"
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>

// Will be merged back into compiz
namespace compiz
{
class PrivateMinimizedWindowHandler;

class MinimizedWindowHandler
{
public:

  MinimizedWindowHandler (Display *dpy, unsigned int xid, compiz::WindowInputRemoverLockAcquireInterface *lock_acquire);
  virtual ~MinimizedWindowHandler ();

  virtual void minimize   ();
  virtual void unminimize ();

  void setVisibility (bool visible, Window shapeWin);

  typedef boost::shared_ptr<MinimizedWindowHandler> Ptr;

  bool contains (boost::shared_ptr <MinimizedWindowHandler> mw);

protected:
  virtual std::vector<unsigned int> getTransients ();

private:

  PrivateMinimizedWindowHandler *priv;
};
}

#endif
