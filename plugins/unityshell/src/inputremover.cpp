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

#include "inputremover.h"
#include <cstdio>

compiz::WindowInputRemover::WindowInputRemover (Display *dpy,
                                                Window xid) :
  mDpy (dpy),
  mShapeWindow (xid),
  mShapeMask (0),
  mInputRects (NULL),
  mNInputRects (0),
  mInputRectOrdering (0),
  mRemoved (false)
{
}

compiz::WindowInputRemover::~WindowInputRemover ()
{
  if (mRemoved)
    restore ();
}

bool
compiz::WindowInputRemover::save ()
{
  XRectangle   *rects;
  int          count = 0, ordering;
  Window       root;
  int          x, y;
  unsigned int width, height, border, depth;

  /* FIXME: There should be a generic GetGeometry request we can
     * use here in order to avoid a round-trip */
  if (!XGetGeometry (mDpy, mShapeWindow, &root, &x, &y, &width, &height,
                     &border, &depth))
  {
    return false;
  }

  rects = XShapeGetRectangles (mDpy, mShapeWindow, ShapeInput,
                               &count, &ordering);
  if (count == 0)
    return false;

  /* check if the returned shape exactly matches the window shape -
   *      if that is true, the window currently has no set input shape */
  if ((count == 1) &&
      (rects[0].x == -((int) border)) &&
      (rects[0].y == -((int) border)) &&
      (rects[0].width == (width + border)) &&
      (rects[0].height == (height + border)))
  {
    count = 0;
  }

  if (mInputRects)
    XFree (mInputRects);

  mShapeMask = XShapeInputSelected (mDpy, mShapeWindow);

  mInputRects = rects;
  mNInputRects = count;
  mInputRectOrdering = ordering;

  return true;
}

bool
compiz::WindowInputRemover::remove ()
{
  if (!mNInputRects)
    save ();

  XShapeSelectInput (mDpy, mShapeWindow, NoEventMask);

  XShapeCombineRectangles (mDpy, mShapeWindow, ShapeInput, 0, 0,
                           NULL, 0, ShapeSet, 0);

  XShapeSelectInput (mDpy, mShapeWindow, ShapeNotify);

  mRemoved = true;
  return true;
}

bool
compiz::WindowInputRemover::restore ()
{
  if (mNInputRects)
  {
    XShapeCombineRectangles (mDpy, mShapeWindow, ShapeInput, 0, 0,
                             mInputRects, mNInputRects,
                             ShapeSet, mInputRectOrdering);
  }
  else
  {
    XShapeCombineMask (mDpy, mShapeWindow, ShapeInput,
                       0, 0, None, ShapeSet);
  }

  if (mInputRects)
    XFree (mInputRects);

  XShapeSelectInput (mDpy, mShapeWindow, mShapeMask);

  mRemoved = false;
  return true;
}
