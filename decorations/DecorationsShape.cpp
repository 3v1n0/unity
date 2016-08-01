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

#include "DecorationsShape.h"

#include <core/core.h>
#include <NuxCore/Logger.h>
#include <X11/extensions/shape.h>

namespace unity
{
namespace decoration
{
namespace
{
DECLARE_LOGGER(logger, "unity.decoration.shape");
}

Shape::Shape(Window xid)
{
  Bool buse, cuse;
  int bx, by, cx, cy;
  unsigned int bw, bh, cw, ch;
  Display *dpy = screen->dpy();

  XShapeQueryExtents(dpy, xid, &buse, &bx, &by, &bw, &bh, &cuse, &cx, &cy, &cw, &ch);

  int kind;

  if (buse)
  {
    width_ = bw;
    height_ = bh;
    xoffs_ = bx;
    yoffs_ = by;
    kind = ShapeBounding;
  }
  else if (cuse)
  {
    width_ = cw;
    height_ = ch;
    xoffs_ = cx;
    yoffs_ = cy;
    kind = ShapeClip;
  }
  else
  {
    LOG_ERROR(logger) << "XShapeQueryExtend returned no extents";
    return;
  }

  int rect_count, rect_order;
  std::unique_ptr<XRectangle[], int(*)(void*)> rectangles(XShapeGetRectangles(dpy, xid, kind, &rect_count, &rect_order), XFree);

  if (!rectangles)
  {
    LOG_ERROR(logger) << "Failed to get shape rectangles";
    return;
  }

  for (int i = 0; i < rect_count; ++i)
    rectangles_.push_back(rectangles[i]);
}

std::vector<XRectangle> const& Shape::GetRectangles() const
{
  return rectangles_;
}

int Shape::Width() const
{
  return width_;
}

int Shape::Height() const
{
  return height_;
}

int Shape::XOffset() const
{
  return xoffs_;
}

int Shape::YOffset() const
{
  return yoffs_;
}

} // decoration namespace
} // unity namespace
