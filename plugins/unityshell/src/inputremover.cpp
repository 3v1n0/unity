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

#include <cstdlib>
#include <boost/scoped_array.hpp>
#include "inputremover.h"
#include <X11/Xregion.h>
#include <cstdio>
#include <cstring>

namespace
{
const unsigned int propVersion = 2;

void CheckRectanglesCount(XRectangle *rects,
                          int        *count,
                          unsigned int width,
                          unsigned int height,
                          unsigned int border)
{
  /* check if the returned shape exactly matches the window shape -
   * if that is true, the window currently has no set input shape */
  if ((*count == 1) &&
      (rects[0].x == -((int) border)) &&
      (rects[0].y == -((int) border)) &&
      (rects[0].width == (width + border)) &&
      (rects[0].height == (height + border)))
  {
    *count = 0;
  }
}

bool CheckWindowExists(Display *dpy, Window shapeWindow,
                       unsigned int *width,
                       unsigned int *height,
                       unsigned int *border)
{
  Window       root;
  int          x, y;
  unsigned int depth;


  /* FIXME: There should be a generic GetGeometry request we can
     * use here in order to avoid a round-trip */
  if (!XGetGeometry (dpy, shapeWindow, &root, &x, &y, width, height,
                     border, &depth))
  {
    return false;
  }

  return true;
}

XRectangle * QueryRectangles(Display *dpy, Window shapeWindow,
                             int        *count,
                             int        *ordering,
                             int        kind)
{
  return XShapeGetRectangles (dpy, shapeWindow, kind,
                              count, ordering);
}

}

compiz::WindowInputRemoverInterface::~WindowInputRemoverInterface ()
{
}

compiz::WindowInputRemover::WindowInputRemover (Display *dpy,
                                                Window shapeWindow,
                                                Window propWindow) :
  mDpy (dpy),
  mShapeWindow (shapeWindow),
  mPropWindow (propWindow),
  mShapeMask (0),
  mInputRects (NULL),
  mNInputRects (0),
  mInputRectOrdering (0),
  mRemoved (false)
{
  /* FIXME: roundtrip */
  XShapeQueryExtension (mDpy, &mShapeEvent, &mShapeError);

  /* Check to see if the propWindow has a saved shape,
   * if so, it means that we are coming from a restart or
   * a crash where it wasn't properly restored, so we need
   * to restore the saved input shape before doing anything
   */
  XRectangle   *rects;
  int          count = 0, ordering;

  if (queryProperty(&rects, &count, &ordering))
  {
    bool rectangles_restored = false;
    unsigned int width, height, border;

    if (CheckWindowExists(mDpy, mShapeWindow, &width, &height, &border))
      if (checkRectangles(rects, &count, ordering,
                          width, height, border))
        if (saveRectangles(rects, count, ordering))
        {
          /* Tell the shape restore engine that we've got a removed
           * input shape here */
          mRemoved = true;
          if (restoreInput())
            rectangles_restored = true;
        }

    /* Something failed and we couldn't restore the
     * rectangles. Don't leak them */
    if (!rectangles_restored)
    {
      free (rects);
    }
  }

}

compiz::WindowInputRemover::~WindowInputRemover ()
{
  if (mRemoved)
    restore ();

  /* Remove the window property as we have already successfully restored
   * the window shape */
  clearProperty();
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

    xsev.x = 0;
    xsev.y = 0;
    xsev.width = 0;
    xsev.height = 0;
    xsev.shaped = true;

    xsev.kind = ShapeInput;

    /* ShapeInput is null */

    xsev.time = CurrentTime;

    XSendEvent (mDpy, mShapeWindow, FALSE, NoEventMask, xev);
    XSendEvent (mDpy, parentReturn, FALSE, NoEventMask, xev);

  }

}

bool
compiz::WindowInputRemover::checkRectangles(XRectangle *input,
                                            int *nInput,
                                            int inputOrdering,
                                            unsigned int width,
                                            unsigned int height,
                                            unsigned int border)
{
  CheckRectanglesCount(input, nInput, width, height, border);

  /* There may be other sanity checks in future */
  return true;
}

bool
compiz::WindowInputRemover::queryShapeRectangles(XRectangle **input,
                                                 int *nInput,
                                                 int *inputOrdering,
                                                 unsigned int *width,
                                                 unsigned int *height,
                                                 unsigned int *border)
{

  if (!CheckWindowExists(mDpy, mShapeWindow, width, height, border))
    return false;

  *input = QueryRectangles(mDpy, mShapeWindow,
                           nInput, inputOrdering, ShapeInput);

  return true;
}

bool
compiz::WindowInputRemover::saveRectangles(XRectangle *input,
                                           int nInput,
                                           int inputOrdering)
{
  if (mInputRects)
    XFree (mInputRects);

  mInputRects = input;
  mNInputRects = nInput;
  mInputRectOrdering = inputOrdering;

  return true;
}

void
compiz::WindowInputRemover::clearRectangles ()
{
  /* Revert save action to local memory */
  if (mInputRects)
    XFree (mInputRects);

  mNInputRects = 0;
  mInputRectOrdering = 0;

  mShapeMask = 0;

  mRemoved = false;
}

bool
compiz::WindowInputRemover::writeProperty (XRectangle *input,
                                           int nInput,
                                           int inputOrdering)
{
  Atom prop = XInternAtom (mDpy, "_UNITY_SAVED_WINDOW_SHAPE", FALSE);
  Atom type = XA_CARDINAL;
  int  fmt  = 32;

  const unsigned int headerSize = 3;

  /*
   * -> version
   * -> nInput
   * -> inputOrdering
   * -> nBounding
   * -> boundingOrdering
   *
   * +
   *
   * nRectangles * 4
   */
  const size_t dataSize = headerSize + (nInput * 4);

  boost::scoped_array<unsigned long> data(new unsigned long[dataSize]);

  data[0] = propVersion;
  data[1] = nInput;
  data[2] = inputOrdering;

  for (int i = 0; i < nInput; ++i)
  {
    const unsigned int position = dataSize + (i * 4);

    data[position + 0] = input[i].x;
    data[position + 1] = input[i].y;
    data[position + 2] = input[i].width;
    data[position + 3] = input[i].height;
  }

  /* No need to check return code, always returns 0 */
  XChangeProperty(mDpy,
                  mPropWindow,
                  prop,
                  type,
                  fmt,
                  PropModeReplace,
                  reinterpret_cast<unsigned char*>(data.get()),
                  dataSize);

  return true;
}

bool
compiz::WindowInputRemover::queryProperty(XRectangle **input,
                                          int *nInput,
                                          int *inputOrdering)

{
  Atom prop = XInternAtom (mDpy, "_UNITY_SAVED_WINDOW_SHAPE", FALSE);
  Atom type = XA_CARDINAL;
  int  fmt  = 32;

  Atom actualType;
  int actualFmt;

  unsigned long nItems;
  unsigned long nLeft;

  unsigned char *propData;

  const unsigned long headerLength = 3L;

  /* First query the first five bytes to figure out how
   * long the rest of the property is going to be */
  if (!XGetWindowProperty(mDpy,
                          mPropWindow,
                          prop,
                          0L,
                          headerLength,
                          FALSE,
                          type,
                          &actualType,
                          &actualFmt,
                          &nItems,
                          &nLeft,
                          &propData))
  {
    return false;
  }

  /* If type or format is mismatched, return false */
  if (actualType != type ||
      actualFmt != fmt ||
      headerLength != nItems)
  {
    XFree (propData);
    return false;
  }

  /* XXX: the cast to void * before the reinterpret_cast is a hack to calm down
   *  gcc on ARM machines and its misalignment cast errors */
  unsigned long *headerData = reinterpret_cast<unsigned long *>(static_cast<void *>(propData));

  /* If version is mismatched, return false */
  if (headerData[0] != propVersion)
    return false;

  /* New length is nInput * 4 + nBounding * 4 + headerSize */
  unsigned long fullLength = *nInput * 4  + headerLength;

  /* Free data and get the rest */
  XFree (propData);

  if (!XGetWindowProperty(mDpy,
                          mPropWindow,
                          prop,
                          0L,
                          fullLength,
                          FALSE,
                          type,
                          &actualType,
                          &actualFmt,
                          &nItems,
                          &nLeft,
                          &propData))
  {
    return false;
  }

  /* Check to make sure we got everything */
  if (fullLength != nItems)
  {
    printf ("warning, did not get full legnth");
    return false;
  }

  unsigned long *data = reinterpret_cast<unsigned long *>(static_cast<void *>(propData));

  /* Read in the header */
  *nInput = data[1];
  *inputOrdering = data[2];

  /* Read in the rectangles */
  *input = reinterpret_cast<XRectangle *>(calloc(1, sizeof(XRectangle) * *nInput));

  for (int i = 0; i < *nInput; ++i)
  {
    const unsigned int position = headerLength + i * 4;

    (*input)[i].x = data[position + 0];
    (*input)[i].y = data[position + 1];
    (*input[i]).width = data[position + 2];
    (*input[i]).height = data[position + 3];
  }

  XFree (propData);

  return true;
}

void
compiz::WindowInputRemover::clearProperty()
{
  Atom prop = XInternAtom (mDpy, "_UNITY_SAVED_WINDOW_SHAPE", FALSE);

  XDeleteProperty(mDpy, mPropWindow, prop);
}

bool
compiz::WindowInputRemover::saveInput ()
{
  XRectangle   *rects;
  int          count = 0, ordering;
  unsigned int width, height, border;

  /* Never save input for a cleared window */
  if (mRemoved)
    return false;

  if (!queryShapeRectangles(&rects, &count, &ordering,
                            &width, &height, &border))
  {
    clearRectangles ();
    return false;
  }

  if (!checkRectangles(rects, &count, ordering,
                       width, height, border))
  {
    clearRectangles ();
    return false;
  }

  if (!writeProperty(rects, count, ordering))
  {
    clearRectangles ();
    return false;
  }

  mShapeMask = XShapeInputSelected (mDpy, mShapeWindow);

  saveRectangles(rects, count, ordering);

  return true;
}

bool
compiz::WindowInputRemover::removeInput ()
{
  if (!mNInputRects)
    if (!save ())
      return false;

  XShapeSelectInput (mDpy, mShapeWindow, NoEventMask);

  XShapeCombineRectangles (mDpy, mShapeWindow, ShapeInput, 0, 0,
                           NULL, 0, ShapeSet, 0);

  XShapeSelectInput (mDpy, mShapeWindow, mShapeMask);

  sendShapeNotify ();

  mRemoved = true;

  return true;
}

bool
compiz::WindowInputRemover::restoreInput ()
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
  }

  XShapeSelectInput (mDpy, mShapeWindow, mShapeMask);

  mRemoved = false;

  sendShapeNotify ();

  return true;
}
