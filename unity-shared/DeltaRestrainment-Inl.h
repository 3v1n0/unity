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
#ifndef _UNITY_DELTA_RESTRAINMENT_INL_H
#define _UNITY_DELTA_RESTRAINMENT_INL_H

#include <NuxCore/Rect.h>
#include <NuxCore/Point.h>
#include <Nux/Utils.h>

namespace unity
{
namespace util
{
void restrainDelta (int                 &dx,
                    int                 &dy,
                    const nux::Geometry &rect,
                    const nux::Point    &currentPoint);

void applyDelta (int dx,
                 int dy,
                 int &x,
                 int &y);
}
}

#endif
