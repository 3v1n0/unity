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

#ifndef _COMPIZ_TRANSIENTFORHANDLER_H
#define _COMPIZ_TRANSIENTFORHANDLER_H

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <vector>
#include <list>
#include <string>

// Will be merged back into compiz
namespace compiz
{
class PrivateX11TransientForReader;

class X11TransientForReader
{
public:

  X11TransientForReader (Display *dpy, Window xid);
  virtual ~X11TransientForReader ();

  bool isTransientFor (unsigned int ancestor);
  bool isGroupTransientFor (unsigned int clientLeader);

  std::vector<unsigned int> getTransients ();

  static Atom wmTransientFor;
  static Atom wmClientLeader;

protected:

  virtual unsigned int getAncestor ();

private:

  PrivateX11TransientForReader *priv;
};
}

#endif
