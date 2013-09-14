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
 *              Jay Taoko <jay.taoko@canonical.com>
 *              Marco Trevisan <marco.trevisan@canonical.com>
 */

#include "unity-shared/CairoTexture.h"
#include "QuicklistMenuItemCheckmark.h"

namespace unity
{

QuicklistMenuItemCheckmark::QuicklistMenuItemCheckmark(glib::Object<DbusmenuMenuitem> const& item, NUX_FILE_LINE_DECL)
  : QuicklistMenuItem(QuicklistMenuItemType::CHECK, item, NUX_FILE_LINE_PARAM)
{
  InitializeText();
}

std::string QuicklistMenuItemCheckmark::GetDefaultText() const
{
  return "Check Mark";
}

std::string QuicklistMenuItemCheckmark::GetName() const
{
  return "QuicklistMenuItemCheckmark";
}

void QuicklistMenuItemCheckmark::UpdateTexture()
{
  int width = GetBaseWidth();
  int height = GetBaseHeight();

  nux::CairoGraphics cairoGraphics(CAIRO_FORMAT_ARGB32, width, height);
  std::shared_ptr<cairo_t> cairo_context(cairoGraphics.GetContext(), cairo_destroy);
  cairo_t* cr = cairo_context.get();

  // draw normal, unchecked version
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  cairo_scale(cr, 1.0f, 1.0f);
  cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 1.0f);
  cairo_set_line_width(cr, 1.0f);

  DrawText(cairoGraphics, width, height, nux::color::White);

  _normalTexture[0].Adopt(texture_from_cairo_graphics(cairoGraphics));

  // draw normal, checked version
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  cairo_scale(cr, 1.0f, 1.0f);
  cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 1.0f);
  cairo_set_line_width(cr, 1.0f);

  cairo_save(cr);
  cairo_translate(cr, Align((ITEM_INDENT_ABS - 16.0f + ITEM_MARGIN) / 2.0f),
                      Align((static_cast<double>(height) - 16.0f) / 2.0f));

  cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 1.0f);

  cairo_translate(cr, 3.0f, 1.0f);
  cairo_move_to(cr, 0.0f, 6.0f);
  cairo_line_to(cr, 0.0f, 8.0f);
  cairo_line_to(cr, 4.0f, 12.0f);
  cairo_line_to(cr, 6.0f, 12.0f);
  cairo_line_to(cr, 15.0f, 1.0f);
  cairo_line_to(cr, 15.0f, 0.0f);
  cairo_line_to(cr, 14.0f, 0.0f);
  cairo_line_to(cr, 5.0f, 9.0f);
  cairo_line_to(cr, 1.0f, 5.0f);
  cairo_close_path(cr);
  cairo_fill(cr);

  cairo_restore(cr);

  DrawText(cairoGraphics, width, height, nux::color::White);

  _normalTexture[1].Adopt(texture_from_cairo_graphics(cairoGraphics));

  // draw active/prelight, unchecked version
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  DrawPrelight(cairoGraphics, width, height, nux::color::White);
  DrawText(cairoGraphics, width, height, nux::color::White * 0.0f);

  _prelightTexture[0].Adopt(texture_from_cairo_graphics(cairoGraphics));

  // draw active/prelight, checked version
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  DrawPrelight(cairoGraphics, width, height, nux::color::White);

  cairo_set_source_rgba(cr, 0.0f, 0.0f, 0.0f, 0.0f);

  cairo_save(cr);
  cairo_translate(cr, Align((ITEM_INDENT_ABS - 16.0f + ITEM_MARGIN) / 2.0f),
                      Align((static_cast<double>(height) - 16.0f) / 2.0f));

  cairo_translate(cr, 3.0f, 1.0f);
  cairo_move_to(cr, 0.0f, 6.0f);
  cairo_line_to(cr, 0.0f, 8.0f);
  cairo_line_to(cr, 4.0f, 12.0f);
  cairo_line_to(cr, 6.0f, 12.0f);
  cairo_line_to(cr, 15.0f, 1.0f);
  cairo_line_to(cr, 15.0f, 0.0f);
  cairo_line_to(cr, 14.0f, 0.0f);
  cairo_line_to(cr, 5.0f, 9.0f);
  cairo_line_to(cr, 1.0f, 5.0f);
  cairo_close_path(cr);
  cairo_fill(cr);

  cairo_restore(cr);

  DrawText(cairoGraphics, width, height, nux::color::White * 0.0f);

  _prelightTexture[1].Adopt(texture_from_cairo_graphics(cairoGraphics));
}

}
