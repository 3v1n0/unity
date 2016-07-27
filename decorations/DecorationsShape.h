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

#include "WindowManager.h"
#include "DecoratedWindow.h"

class DecorationsShape
{
private:
  std::vector<XRectangle> rects;
  int width, height;
  int xoffs, yoffs;

public:
  bool initShape(XID win);
  const XRectangle& getRectangle(int idx) const;
  int getRectangleCount() const;
  int getWidth() const;
  int getHeight() const;
  int getXoffs() const;
  int getYoffs() const;
  void clear();
};
#endif //DECORATIONS_SHAPE_H_
