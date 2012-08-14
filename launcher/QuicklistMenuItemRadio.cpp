// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010-2012 Canonical Ltd
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
 * Authored by: Mirco Müller <mirco.mueller@canonical.com>
 *              Marco Trevisan <marco.trevisan@canonical.com>
 */

#include "unity-shared/CairoTexture.h"
#include "QuicklistMenuItemRadio.h"

namespace unity
{

QuicklistMenuItemRadio::QuicklistMenuItemRadio(glib::Object<DbusmenuMenuitem> const& item, NUX_FILE_LINE_DECL)
  : QuicklistMenuItem(QuicklistMenuItemType::RADIO, item, NUX_FILE_LINE_PARAM)
{
  InitializeText();
}

std::string QuicklistMenuItemRadio::GetDefaultText() const
{
  return "Radio Button";
}

std::string QuicklistMenuItemRadio::GetName() const
{
  return "QuicklistMenuItemRadio";
}

void QuicklistMenuItemRadio::UpdateTexture()
{
  int width = GetBaseWidth();
  int height = GetBaseHeight();

  nux::CairoGraphics cairoGraphics(CAIRO_FORMAT_ARGB32, width, height);
  std::shared_ptr<cairo_t> cairo_context(cairoGraphics.GetContext(), cairo_destroy);
  cairo_t* cr = cairo_context.get();

  // draw normal, disabled version
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  DrawText(cairoGraphics, width, height, nux::color::White);

  _normalTexture[0].Adopt(texture_from_cairo_graphics(cairoGraphics));

  // draw normal, enabled version
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  cairo_scale(cr, 1.0f, 1.0f);
  cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 1.0f);
  cairo_set_line_width(cr, 1.0f);

  double x = Align((ITEM_INDENT_ABS + ITEM_MARGIN) / 2.0f);
  double y = Align(static_cast<double>(height) / 2.0f);
  double radius = 3.5f;

  cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 1.0f);
  cairo_arc(cr, x, y, radius, 0.0f * (G_PI / 180.0f), 360.0f * (G_PI / 180.0f));
  cairo_fill(cr);

  DrawText(cairoGraphics, width, height, nux::color::White);

  _normalTexture[1].Adopt(texture_from_cairo_graphics(cairoGraphics));

  // draw active/prelight, unchecked version
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  DrawPrelight(cairoGraphics, width, height, nux::color::White);
  DrawText(cairoGraphics, width, height, nux::color::White * 0.0f);

  _prelightTexture[0].Adopt(texture_from_cairo_graphics(cairoGraphics));

  // draw active/prelight, unchecked version
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  DrawPrelight(cairoGraphics, width, height, nux::color::White);

  cairo_set_source_rgba(cr, 0.0f, 0.0f, 0.0f, 0.0f);

  cairo_arc(cr, x, y, radius, 0.0f * (G_PI / 180.0f), 360.0f * (G_PI / 180.0f));
  cairo_fill(cr);

  DrawText(cairoGraphics, width, height, nux::color::White * 0.0f);

  _prelightTexture[1].Adopt(texture_from_cairo_graphics(cairoGraphics));
}

} // NAMESPACE
