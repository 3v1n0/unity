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

#ifndef DECORATIONS_SHAPE_H_
#define DECORATIONS_SHAPE_H_

#include <X11/Xlib.h>
#include <vector>

namespace unity
{
namespace decoration
{
class Shape
{
public:
  Shape(Window);

  int Width() const;
  int Height() const;
  int XOffset() const;
  int YOffset() const;

  std::vector<XRectangle> const& GetRectangles() const;

private:
  int width_;
  int height_;
  int xoffs_;
  int yoffs_;

  std::vector<XRectangle> rectangles_;
};

} // decoration namespace
} // unity namespace

#endif //DECORATIONS_SHAPE_H_
