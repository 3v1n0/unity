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

#ifndef EM_CONVERTER_H
#define EM_CONVERTER_H

namespace unity
{

class EMConverter
{
public:
  EMConverter(int font_size, double dpi = 96.0f);

  void SetFontSize(int font_size);
  void SetDPI(double dpi);

  int    GetFontSize() const;
  double GetDPI() const;

  int    EMToPixels(double em) const;
  double PixelsToEM(int pixels) const;

private:
  void UpdatePixelsPerEM();

  double pixels_per_em_;
  double dpi_;
  int font_size_;
};

} // namespace unity

#endif // EM_CONVERTER_H
