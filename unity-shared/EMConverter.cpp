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

#include <cmath>
#include "EMConverter.h"

namespace unity
{

namespace
{
const double BASE_DPI = 96.0;
}

EMConverter::EMConverter(double dpi)
  : dpi_(dpi)
{}

bool EMConverter::SetDPI(double dpi)
{
  if (dpi != dpi_)
  {
    dpi_ = dpi;
    return true;
  }

  return false;
}

double EMConverter::GetDPI() const
{
  return dpi_;
}

double EMConverter::CP(int pixels) const
{
  return std::round(pixels * DPIScale());
}

double EMConverter::DPIScale() const
{
  return dpi_ / BASE_DPI;
}

} // namespace unity
