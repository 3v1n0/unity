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
#include "QuicklistMenuItemSeparator.h"

namespace unity
{

QuicklistMenuItemSeparator::QuicklistMenuItemSeparator(glib::Object<DbusmenuMenuitem> const& item, NUX_FILE_LINE_DECL)
  : QuicklistMenuItem(QuicklistMenuItemType::SEPARATOR, item, NUX_FILE_LINE_PARAM)
  , _color(1.0f, 1.0f, 1.0f, 0.5f)
  , _premultiplied_color(0.5f, 0.5f, 0.5f, 0.5f)
{
  SetMinimumHeight(7);
  SetBaseSize(64, 7);
}

std::string QuicklistMenuItemSeparator::GetName() const
{
  return "QuicklistMenuItemSeparator";
}

bool QuicklistMenuItemSeparator::GetSelectable()
{
  return false;
}

void QuicklistMenuItemSeparator::Draw(nux::GraphicsEngine& gfxContext, bool forceDraw)
{
  // Check if the texture have been computed. If they haven't, exit the function.
  if (!_normalTexture[0])
    return;

  nux::Geometry const& base = GetGeometry();

  gfxContext.PushClippingRectangle(base);

  nux::TexCoordXForm texxform;
  texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
  texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);

  gfxContext.GetRenderStates().SetBlend(true);
  gfxContext.GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);

  auto const& texture = _normalTexture[0]->GetDeviceTexture();
  gfxContext.QRP_1Tex(base.x, base.y, base.width, base.height, texture, texxform, _premultiplied_color);

  gfxContext.GetRenderStates().SetBlend(false);
  gfxContext.PopClippingRectangle();
}

void QuicklistMenuItemSeparator::UpdateTexture()
{
  int width = GetBaseWidth();
  int height = GetBaseHeight();

  nux::CairoGraphics cairoGraphics(CAIRO_FORMAT_ARGB32, width, height);
  std::shared_ptr<cairo_t> cairo_context(cairoGraphics.GetContext(), cairo_destroy);
  cairo_t* cr = cairo_context.get();

  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_rgba(cr, 0.0f, 0.0f, 0.0f, 0.0f);
  cairo_paint(cr);
  cairo_set_source_rgba(cr, _color.red, _color.green, _color.blue, _color.alpha);
  cairo_set_line_width(cr, 1.0f);
  cairo_move_to(cr, 0.0f, 3.5f);
  cairo_line_to(cr, width, 3.5f);
  cairo_stroke(cr);

  _normalTexture[0].Adopt(texture_from_cairo_graphics(cairoGraphics));
}

}
