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
#include "QuicklistMenuItemLabel.h"

namespace unity
{

QuicklistMenuItemLabel::QuicklistMenuItemLabel(glib::Object<DbusmenuMenuitem> const& item, NUX_FILE_LINE_DECL)
  : QuicklistMenuItem(QuicklistMenuItemType::LABEL, item, NUX_FILE_LINE_PARAM)
{
  InitializeText();
}

std::string QuicklistMenuItemLabel::GetDefaultText() const
{
  return "Label";
}

std::string QuicklistMenuItemLabel::GetName() const
{
  return "QuicklistMenuItemLabel";
}

void QuicklistMenuItemLabel::UpdateTexture()
{
  int width = GetBaseWidth();
  int height = GetBaseHeight();

  nux::CairoGraphics cairoGraphics(CAIRO_FORMAT_ARGB32, width, height);
  std::shared_ptr<cairo_t> cairo_context(cairoGraphics.GetContext(), cairo_destroy);
  cairo_t* cr = cairo_context.get();

  // draw normal, unchecked version
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  DrawText(cairoGraphics, width, height, nux::color::White);

  _normalTexture[0].Adopt(texture_from_cairo_graphics(cairoGraphics));

  // draw active/prelight, unchecked version
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  DrawPrelight(cairoGraphics, width, height, nux::color::White);
  DrawText(cairoGraphics, width, height, nux::color::White * 0.0f);

  _prelightTexture[0].Adopt(texture_from_cairo_graphics(cairoGraphics));
}

} // NAMESPACE
