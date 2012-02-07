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

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include <Nux/Nux.h>

#include "CairoTexture.h"
#include "QuicklistMenuItemRadio.h"

namespace unity
{

static double
_align(double val)
{
  double fract = val - (int) val;

  if (fract != 0.5f)
    return (double)((int) val + 0.5f);
  else
    return val;
}

QuicklistMenuItemRadio::QuicklistMenuItemRadio(DbusmenuMenuitem* item,
                                               NUX_FILE_LINE_DECL) :
  QuicklistMenuItem(item,
                    NUX_FILE_LINE_PARAM)
{
  _item_type  = MENUITEM_TYPE_RADIO;
  _name = "QuicklistMenuItemRadio";
  InitializeText();
}

QuicklistMenuItemRadio::QuicklistMenuItemRadio(DbusmenuMenuitem* item,
                                               bool              debug,
                                               NUX_FILE_LINE_DECL) :
  QuicklistMenuItem(item,
                    debug,
                    NUX_FILE_LINE_PARAM)
{
  _item_type  = MENUITEM_TYPE_RADIO;
  _name = "QuicklistMenuItemRadio";
  InitializeText();
}

QuicklistMenuItemRadio::~QuicklistMenuItemRadio()
{}

const gchar*
QuicklistMenuItemRadio::GetDefaultText()
{
  return "Radio Button";
}

void
QuicklistMenuItemRadio::PreLayoutManagement()
{
  _pre_layout_width = GetBaseWidth();
  _pre_layout_height = GetBaseHeight();

  if (_normalTexture[0] == 0)
  {
    UpdateTexture();
  }

  QuicklistMenuItem::PreLayoutManagement();
}

long
QuicklistMenuItemRadio::PostLayoutManagement(long layoutResult)
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
QuicklistMenuItemRadio::Draw(nux::GraphicsEngine& gfxContext, bool forceDraw)
{
  nux::ObjectPtr<nux::IOpenGLBaseTexture> texture;

  // Check if the texture have been computed. If they haven't, exit the function.
  if (!_normalTexture[0] || !_prelightTexture[0])
    return;

  nux::Geometry base = GetGeometry();

  gfxContext.PushClippingRectangle(base);

  nux::TexCoordXForm texxform;
  texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
  texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);

  gfxContext.GetRenderStates().SetBlend(true);
  gfxContext.GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);

  unsigned int texture_idx = GetActive() ? 1 : 0;

  if (!_prelight || !GetEnabled())
  {
    texture = _normalTexture[texture_idx]->GetDeviceTexture();
  }
  else
  {
    texture = _prelightTexture[texture_idx]->GetDeviceTexture();
  }

  _color = GetEnabled() ? nux::color::White : nux::color::White * 0.35;

  gfxContext.QRP_1Tex(base.x,
                      base.y,
                      base.width,
                      base.height,
                      texture,
                      texxform,
                      _color);

  gfxContext.GetRenderStates().SetBlend(false);

  gfxContext.PopClippingRectangle();
}

void
QuicklistMenuItemRadio::DrawContent(nux::GraphicsEngine& gfxContext,
                                    bool                 forceDraw)
{
}

void
QuicklistMenuItemRadio::PostDraw(nux::GraphicsEngine& gfxContext,
                                 bool                 forceDraw)
{
}

void
QuicklistMenuItemRadio::UpdateTexture()
{
  int        width       = GetBaseWidth();
  int        height      = GetBaseHeight();

  _cairoGraphics = new nux::CairoGraphics(CAIRO_FORMAT_ARGB32, width, height);
  cairo_t* cr = _cairoGraphics->GetContext();

  // draw normal, disabled version
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  DrawText(_cairoGraphics, width, height, nux::color::White);

  if (_normalTexture[0])
    _normalTexture[0]->UnReference();

  _normalTexture[0] = texture_from_cairo_graphics(*_cairoGraphics);

  // draw normal, enabled version
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  cairo_scale(cr, 1.0f, 1.0f);
  cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 1.0f);
  cairo_set_line_width(cr, 1.0f);

  double x      = _align((ITEM_INDENT_ABS + ITEM_MARGIN) / 2.0f);
  double y      = _align((double) height / 2.0f);
  double radius = 3.5f;

  cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 1.0f);
  cairo_arc(cr, x, y, radius, 0.0f * (G_PI / 180.0f), 360.0f * (G_PI / 180.0f));
  cairo_fill(cr);

  DrawText(_cairoGraphics, width, height, nux::color::White);

  if (_normalTexture[1])
    _normalTexture[1]->UnReference();

  _normalTexture[1] = texture_from_cairo_graphics(*_cairoGraphics);

  // draw active/prelight, unchecked version
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  DrawPrelight(_cairoGraphics, width, height, nux::color::White);
  DrawText(_cairoGraphics, width, height, nux::color::White * 0.0f);

  if (_prelightTexture[0])
    _prelightTexture[0]->UnReference();

  _prelightTexture[0] = texture_from_cairo_graphics(*_cairoGraphics);

  // draw active/prelight, unchecked version
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  DrawPrelight(_cairoGraphics, width, height, nux::color::White);

  cairo_set_source_rgba(cr, 0.0f, 0.0f, 0.0f, 0.0f);

  cairo_arc(cr, x, y, radius, 0.0f * (G_PI / 180.0f), 360.0f * (G_PI / 180.0f));
  cairo_fill(cr);

  DrawText(_cairoGraphics, width, height, nux::color::White * 0.0f);

  if (_prelightTexture[1])
    _prelightTexture[1]->UnReference();

  _prelightTexture[1] = texture_from_cairo_graphics(*_cairoGraphics);

  // finally clean up
  cairo_destroy(cr);
  delete _cairoGraphics;
}

int QuicklistMenuItemRadio::CairoSurfaceWidth()
{
  if (_normalTexture[0])
    return _normalTexture[0]->GetWidth();

  return 0;
}

} // NAMESPACE
