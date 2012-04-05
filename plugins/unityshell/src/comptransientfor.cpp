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

#include "comptransientfor.h"

namespace compiz
{
class PrivateCompTransientForReader
{
public:

  PrivateCompTransientForReader () {};

  CompWindow *mWindow;
};
}

compiz::CompTransientForReader::CompTransientForReader (CompWindow *w) :
  X11TransientForReader (screen->dpy (), w->id ())
{
  priv = new PrivateCompTransientForReader ();

  priv->mWindow = w;
}

compiz::CompTransientForReader::~CompTransientForReader ()
{
  delete priv;
}

unsigned int
compiz::CompTransientForReader::getAncestor ()
{
  return priv->mWindow->transientFor ();
}

bool
compiz::CompTransientForReader::isTransientFor (unsigned int ancestor)
{
  if (!ancestor ||
      !priv->mWindow->id ())
      return false;

  return priv->mWindow->transientFor () == ancestor;
}

bool
compiz::CompTransientForReader::isGroupTransientFor (unsigned int clientLeader)
{
  if (!clientLeader ||
      !priv->mWindow->id ())
    return false;

  if (priv->mWindow->transientFor () == None || priv->mWindow->transientFor () == screen->root ())
  {
    if (priv->mWindow->type () & (CompWindowTypeUtilMask    |
                                  CompWindowTypeToolbarMask |
                                  CompWindowTypeMenuMask    |
                                  CompWindowTypeDialogMask  |
                                  CompWindowTypeModalDialogMask))
    {
	    if (priv->mWindow->clientLeader () == clientLeader)
        return true;
    }
  }

  return false;
}
