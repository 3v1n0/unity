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

#include "EMConverter.h"

namespace unity
{

double const BASE_DPI        = 96.0;
double const DEFAULT_PPE     = 10.0;
double const PIXELS_PER_INCH = 72.0;

EMConverter::EMConverter(int font_size, double dpi)
  : pixels_per_em_(DEFAULT_PPE)
  , base_pixels_per_em_(DEFAULT_PPE)
  , dpi_(dpi)
  , font_size_(font_size)
{
  UpdatePixelsPerEM();
  UpdateBasePixelsPerEM();
}

void EMConverter::UpdatePixelsPerEM()
{
  pixels_per_em_ = font_size_ * dpi_ / PIXELS_PER_INCH;

  if (pixels_per_em_ == 0)
    pixels_per_em_ = DEFAULT_PPE;
}

void EMConverter::UpdateBasePixelsPerEM()
{
  base_pixels_per_em_ = font_size_ * BASE_DPI / PIXELS_PER_INCH;

  if (base_pixels_per_em_ == 0)
    base_pixels_per_em_ = DEFAULT_PPE;
}

void EMConverter::SetFontSize(int font_size)
{
  if (font_size != font_size_)
  {
    font_size_ = font_size;
    UpdatePixelsPerEM();
    UpdateBasePixelsPerEM();
  }
}

void EMConverter::SetDPI(double dpi)
{
  if (dpi != dpi_)
  {
    dpi_ = dpi;
    UpdatePixelsPerEM();
  }
}

int EMConverter::GetFontSize() const
{
  return font_size_;
}

double EMConverter::GetDPI() const
{
  return dpi_;
}

int EMConverter::EMToPixels(double em) const
{
  return (em * pixels_per_em_);
}

double EMConverter::PixelsToBaseEM(int pixels) const
{
  return (pixels / base_pixels_per_em_);
}

int EMConverter::ConvertPixels(int pixels) const
{
  double pixels_em = PixelsToBaseEM(pixels);
  return EMToPixels(pixels_em);
}

double EMConverter::DPIScale() const
{
  return dpi_ / BASE_DPI;
}

} // namespace unity
