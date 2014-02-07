// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2014 Canonical Ltd
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
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#ifndef RAW_PIXEL_H
#define RAW_PIXEL_H

#include "EMConverter.h"

namespace unity
{

class RawPixel
{
public:
  RawPixel(float raw_pixel);

  float CP(EMConverter const& converter) const;

  operator float() const;

private:
  float raw_pixel_;

};

// User-Defined Literals (ex: 10_em, 10.0_em)
RawPixel operator"" _em(long double pixel);
RawPixel operator"" _em(unsigned long long pixel);

} // namespace unity

#endif // RAW_PIXEL_H
