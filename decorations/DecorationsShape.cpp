// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2016 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Eleni Maria Stea <elenimaria.stea@canonical.com>
 */

#include <string.h>
#include <X11/extensions/shape.h>
#include "DecoratedWindow.h"
#include "DecorationsShape.h"

bool DecorationsShape::initShape(XID win)
{
  Bool buse, cuse;
  int bx, by, cx, cy;
  unsigned int bw, bh, cw, ch;
  Display *dpy = screen->dpy();

  XShapeQueryExtents(dpy, win, &buse, &bx, &by, &bw, &bh, &cuse, &cx, &cy, &cw, &ch);

  int kind;
  if (buse) {
    width = bw;
    height = bh;
    xoffs = bx;
    yoffs = by;
    kind = ShapeBounding;
  }
  else if (cuse) {
    width = cw;
    height = ch;
    xoffs = cx;
    yoffs = cy;
    kind = ShapeClip;
  }
  else {
    fprintf(stderr, "XShapeQueryExtend returned no extends.\n");
    return false;
  }

  int rect_count, rect_order;
  XRectangle *rectangles;
  if (!(rectangles = XShapeGetRectangles(dpy, win, kind, &rect_count, &rect_order))) {
    fprintf(stderr, "Failed to get shape rectangles\n");
    return false;
  }

  for (int i=0; i< rect_count; i++) {
    rects.push_back(rectangles[i]);
  }

  XFree(rectangles);
  return true;
}

const XRectangle& DecorationsShape::getRectangle(int idx) const
{
  return rects[idx];
}

int DecorationsShape::getRectangleCount() const
{
  return (int)rects.size();
}

int DecorationsShape::getWidth() const
{
  return width;
}

int DecorationsShape::getHeight() const
{
  return height;
}

int DecorationsShape::getXoffs() const
{
  return xoffs;
}

int DecorationsShape::getYoffs() const
{
  return yoffs;
}
void DecorationsShape::clear()
{
  width = height = 0;
  rects.clear();
}
