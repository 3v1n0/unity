// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010 Canonical Ltd
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
 */

#include <Nux/Nux.h>

#include "unity-shared/CairoTexture.h"
#include "QuicklistMenuItemSeparator.h"

namespace unity
{

QuicklistMenuItemSeparator::QuicklistMenuItemSeparator(DbusmenuMenuitem* item,
                                                       NUX_FILE_LINE_DECL) :
  QuicklistMenuItem(item,
                    NUX_FILE_LINE_PARAM)
{
  _name = "QuicklistMenuItemSeparator";
  SetMinimumHeight(7);
  SetBaseSize(64, 7);

  _color      = nux::Color(1.0f, 1.0f, 1.0f, 0.5f);
  _premultiplied_color = nux::Color(0.5f, 0.5f, 0.5f, 0.5f);
  _item_type  = MENUITEM_TYPE_SEPARATOR;
}

QuicklistMenuItemSeparator::QuicklistMenuItemSeparator(DbusmenuMenuitem* item,
                                                       bool              debug,
                                                       NUX_FILE_LINE_DECL) :
  QuicklistMenuItem(item,
                    debug,
                    NUX_FILE_LINE_PARAM)
{
  _name = "QuicklistMenuItemSeparator";
  SetMinimumHeight(7);
  SetBaseSize(64, 7);
  //_normalTexture = NULL;
  _color      = nux::Color(1.0f, 1.0f, 1.0f, 0.5f);
  _premultiplied_color = nux::Color(0.5f, 0.5f, 0.5f, 0.5f);
  _item_type  = MENUITEM_TYPE_SEPARATOR;
}

QuicklistMenuItemSeparator::~QuicklistMenuItemSeparator()
{
}

bool
QuicklistMenuItemSeparator::GetSelectable()
{
  return false;
}

void
QuicklistMenuItemSeparator::PreLayoutManagement()
{
  _pre_layout_width = GetBaseWidth();
  _pre_layout_height = GetBaseHeight();

  if ((_normalTexture[0] == 0))
  {
    UpdateTexture();
  }

  QuicklistMenuItem::PreLayoutManagement();
}

long
QuicklistMenuItemSeparator::PostLayoutManagement(long layoutResult)
{
  int w = GetBaseWidth();
  int h = GetBaseHeight();

  long result = 0;

  if (_pre_layout_width < w)
    result |= nux::eLargerWidth;
  else if (_pre_layout_width > w)
    result |= nux::eSmallerWidth;
  else
    result |= nux::eCompliantWidth;

  if (_pre_layout_height < h)
    result |= nux::eLargerHeight;
  else if (_pre_layout_height > h)
    result |= nux::eSmallerHeight;
  else
    result |= nux::eCompliantHeight;

  return result;
}

void
QuicklistMenuItemSeparator::Draw(nux::GraphicsEngine& gfxContext,
                                 bool                 forceDraw)
{
  // Check if the texture have been computed. If they haven't, exit the function.
  if (_normalTexture[0] == 0)
    return;

  nux::Geometry base = GetGeometry();

  gfxContext.PushClippingRectangle(base);

  nux::TexCoordXForm texxform;
  texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
  texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);

  gfxContext.GetRenderStates().SetBlend(true);
  gfxContext.GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);

  gfxContext.QRP_1Tex(base.x,
                      base.y,
                      base.width,
                      base.height,
                      _normalTexture[0]->GetDeviceTexture(),
                      texxform,
                      _premultiplied_color);

  gfxContext.GetRenderStates().SetBlend(false);

  gfxContext.PopClippingRectangle();
}

void
QuicklistMenuItemSeparator::DrawContent(nux::GraphicsEngine& gfxContext,
                                        bool                 forceDraw)
{
}

void
QuicklistMenuItemSeparator::PostDraw(nux::GraphicsEngine& gfxContext,
                                     bool                 forceDraw)
{
}

void
QuicklistMenuItemSeparator::UpdateTexture()
{
  int width  = GetBaseWidth();
  int height  = GetBaseHeight();

  _cairoGraphics = new nux::CairoGraphics(CAIRO_FORMAT_ARGB32, width, height);
  cairo_t* cr = _cairoGraphics->GetContext();

  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_rgba(cr, 0.0f, 0.0f, 0.0f, 0.0f);
  cairo_paint(cr);
  cairo_set_source_rgba(cr, _color.red, _color.green, _color.blue, _color.alpha);
  cairo_set_line_width(cr, 1.0f);
  cairo_move_to(cr, 0.0f, 3.5f);
  cairo_line_to(cr, width, 3.5f);
  cairo_stroke(cr);

  if (_normalTexture[0])
    _normalTexture[0]->UnReference();

  _normalTexture[0] = texture_from_cairo_graphics(*_cairoGraphics);

  cairo_destroy(cr);
  delete _cairoGraphics;
}

int QuicklistMenuItemSeparator::CairoSurfaceWidth()
{
  if (_normalTexture[0])
    return _normalTexture[0]->GetWidth();

  return 0;
}

}
