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
#include <X11/Xregion.h>
#include <cstdio>
#include <cstring>

compiz::WindowInputRemover::WindowInputRemover (Display *dpy,
                                                Window xid) :
  mDpy (dpy),
  mShapeWindow (xid),
  mShapeMask (0),
  mInputRects (NULL),
  mNInputRects (0),
  mInputRectOrdering (0),
  mBoundingRects (NULL),
  mNBoundingRects (0),
  mBoundingRectOrdering (0),
  mRemoved (false)
{
  /* FIXME: roundtrip */
  XShapeQueryExtension (mDpy, &mShapeEvent, &mShapeError);
}

compiz::WindowInputRemover::~WindowInputRemover ()
{
  if (mRemoved)
    restore ();
}

void
compiz::WindowInputRemover::sendShapeNotify ()
{
  /* Send a synthetic ShapeNotify event to the client and parent window
   * since we ignored shape events when setting visibility
   * in order to avoid cycling in the shape handling code -
   * ignore the sent shape notify event since that will
   * be send_event = true
   *
   * NB: We must send ShapeNotify events to both the client
   * window and to the root window with SubstructureRedirectMask
   * since NoEventMask will only deliver the event to the client
   * (see xserver/dix/events.c on the handling of CantBeFiltered
   *  messages)
   *
   * NB: This code will break if you don't have this patch on the
   * X Server since XSendEvent for non-core events are broken.
   *
   * http://lists.x.org/archives/xorg-devel/2011-September/024996.html
   */

  XShapeEvent  xsev;
  XEvent       *xev = (XEvent *) &xsev;
  Window       rootReturn, parentReturn;
  Window       childReturn;
  Window       *children;
  int          x, y, xOffset, yOffset;
  unsigned int width, height, depth, border, nchildren;

  memset (&xsev, 0, sizeof (XShapeEvent));

  /* XXX: libXShape is weird and range checks the event
   * type on event_to_wire so ensure that we are setting
   * the event type on the right range */
  xsev.type = (mShapeEvent - ShapeNotify) & 0x7f;
  /* We must explicitly fill in these values to avoid padding errors */
  xsev.serial = 0L;
  xsev.send_event = TRUE;
  xsev.display = mDpy;
  xsev.window = mShapeWindow;

  if (!mRemoved)
  {
    /* FIXME: these roundtrips suck */
    if (!XGetGeometry (mDpy, mShapeWindow, &rootReturn, &x, &y, &width, &height, &depth, &border))
      return;
    if (!XQueryTree (mDpy, mShapeWindow, &rootReturn, &parentReturn, &children, &nchildren))
      return;

    /* We need to translate the co-ordinates of the origin to the
     * client window to its parent to find out the offset of its
     * position so that we can subtract that from the final bounding
     * rect of the window shape according to the Shape extension
     * specification */

    XTranslateCoordinates (mDpy, mShapeWindow, parentReturn, 0, 0,
			   &xOffset, &yOffset, &childReturn);

    xsev.kind = ShapeBounding;

    /* Calculate extents of the bounding shape */
    if (!mNBoundingRects)
    {
      /* No set input shape, we must use the client geometry */
      xsev.x = x - xOffset;
      xsev.y = y - yOffset;
      xsev.width = width; 
      xsev.height = height;
      xsev.shaped = false;
    }
    else
    {
      Region      boundingRegion = XCreateRegion ();

      for (int i = 0; i < mNBoundingRects; i++)
        XUnionRectWithRegion (&(mBoundingRects[i]), boundingRegion, boundingRegion);

      xsev.x = boundingRegion->extents.x1 - xOffset;
      xsev.y = boundingRegion->extents.y1 - yOffset;
      xsev.width = boundingRegion->extents.x2 - boundingRegion->extents.x1;
      xsev.height = boundingRegion->extents.y2 - boundingRegion->extents.y1;
      xsev.shaped = true;

      XDestroyRegion (boundingRegion);
    }

    xsev.time = CurrentTime;

    XSendEvent (mDpy, mShapeWindow, FALSE, NoEventMask, xev);
    XSendEvent (mDpy, parentReturn, FALSE, NoEventMask, xev);
    xsev.kind = ShapeInput;

    /* Calculate extents of the bounding shape */
    if (!mNInputRects)
    {
      /* No set input shape, we must use the client geometry */
      xsev.x = x - xOffset;
      xsev.y = y - yOffset;
      xsev.width = width; 
      xsev.height = height;
      xsev.shaped = false;
    }
    else
    {
      Region      inputRegion = XCreateRegion ();

      for (int i = 0; i < mNInputRects; i++)
        XUnionRectWithRegion (&(mInputRects[i]), inputRegion, inputRegion);

      xsev.x = inputRegion->extents.x1 - xOffset;
      xsev.y = inputRegion->extents.y1 - yOffset;
      xsev.width = inputRegion->extents.x2 - inputRegion->extents.x1;
      xsev.height = inputRegion->extents.y2 - inputRegion->extents.y1;
      xsev.shaped = true;

      XDestroyRegion (inputRegion);
    }

    xsev.time = CurrentTime;

    XSendEvent (mDpy, mShapeWindow, FALSE, NoEventMask, xev);
    XSendEvent (mDpy, parentReturn, FALSE, NoEventMask, xev);

    if (children)
      XFree (children);
  }
  else
  {
    XQueryTree (mDpy, mShapeWindow, &rootReturn, &parentReturn, &children, &nchildren);

    xsev.kind = ShapeBounding;

    xsev.x = 0;
    xsev.y = 0;
    xsev.width = 0;
    xsev.height = 0;
    xsev.shaped = true;

    xsev.time = CurrentTime;
    XSendEvent (mDpy, mShapeWindow, FALSE, NoEventMask, xev);
    XSendEvent (mDpy, parentReturn, FALSE, NoEventMask, xev);

    xsev.kind = ShapeInput;

    /* Both ShapeBounding and ShapeInput are null */

    xsev.time = CurrentTime;

    XSendEvent (mDpy, mShapeWindow, FALSE, NoEventMask, xev);
    XSendEvent (mDpy, parentReturn, FALSE, NoEventMask, xev);

  }

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

  /* check if the returned shape exactly matches the window shape -
   * if that is true, the window currently has no set input shape */
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

  mInputRects = rects;
  mNInputRects = count;
  mInputRectOrdering = ordering;

  rects = XShapeGetRectangles (mDpy, mShapeWindow, ShapeBounding,
                               &count, &ordering);

  /* check if the returned shape exactly matches the window shape -
   * if that is true, the window currently has no set bounding shape */
  if ((count == 1) &&
      (rects[0].x == -((int) border)) &&
      (rects[0].y == -((int) border)) &&
      (rects[0].width == (width + border)) &&
      (rects[0].height == (height + border)))
  {
    count = 0;
  }

  if (mBoundingRects)
    XFree (mBoundingRects);

  mBoundingRects = rects;
  mNBoundingRects = count;
  mBoundingRectOrdering = ordering;

  mShapeMask = XShapeInputSelected (mDpy, mShapeWindow);

  return true;
}

bool
compiz::WindowInputRemover::remove ()
{
  if (!mNInputRects)
    if (!save ())
      return false;

  XShapeSelectInput (mDpy, mShapeWindow, NoEventMask);

  XShapeCombineRectangles (mDpy, mShapeWindow, ShapeInput, 0, 0,
                           NULL, 0, ShapeSet, 0);
  XShapeCombineRectangles (mDpy, mShapeWindow, ShapeBounding, 0, 0,
                           NULL, 0, ShapeSet, 0);

  XShapeSelectInput (mDpy, mShapeWindow, mShapeMask);

  mRemoved = true;

  sendShapeNotify ();

  return true;
}

bool
compiz::WindowInputRemover::restore ()
{
  XShapeSelectInput (mDpy, mShapeWindow, NoEventMask);

  if (mRemoved)
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
    {
      XFree (mInputRects);
      mInputRects = NULL;
      mNInputRects = 0;
    }

    if (mNBoundingRects)
    {
      XShapeCombineRectangles (mDpy, mShapeWindow, ShapeBounding, 0, 0,
                         mBoundingRects, mNBoundingRects,
                         ShapeSet, mBoundingRectOrdering);
    }
    else
    {
      XShapeCombineMask (mDpy, mShapeWindow, ShapeBounding,
                   0, 0, None, ShapeSet);
    }

    if (mBoundingRects)
    {
      XFree (mBoundingRects);
      mBoundingRects = NULL;
      mNBoundingRects = 0;
    }
  }

  XShapeSelectInput (mDpy, mShapeWindow, mShapeMask);

  mRemoved = false;

  sendShapeNotify ();

  return true;
}
