// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/* Compiz unity plugin
 * unity.cpp
 *
 * Copyright (c) 2010-11 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Your own copyright notice would go above. You are free to choose whatever
 * licence you want, just take note that some compiz code is GPL and you will
 * not be able to re-use it if you want to use a different licence.
 */
#include <iostream>
#include <algorithm>
#include "DeltaRestrainment-Inl.h"

void
unity::util::restrainDelta(int                 &dx,
			   int                 &dy,
			   nux::Geometry const &rect,
			   nux::Point    const &currentPoint)
{
  int restrain_x = std::min(0, (rect.x + rect.width) - (currentPoint.x + dx));
  int restrain_y = std::min(0, (rect.y + rect.height) - (currentPoint.y + dy));

  restrain_x += rect.x - std::min(rect.x, currentPoint.x + dx);
  restrain_y += rect.y - std::min(rect.y, currentPoint.y + dy);

  dx += restrain_x;
  dy += restrain_y;
}
