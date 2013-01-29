// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011 Canonical Ltd
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
#include "PlacesVScrollBar.h"

using unity::texture_from_cairo_graphics;

namespace unity
{
namespace dash
{

PlacesVScrollBar::PlacesVScrollBar(NUX_FILE_LINE_DECL)
  : VScrollBar(NUX_FILE_LINE_PARAM),
    _slider_texture(NULL)
{
  _scroll_up_button->SetMaximumHeight(15);
  _scroll_up_button->SetMinimumHeight(15);

  _scroll_down_button->SetMaximumHeight(15);
  _scroll_down_button->SetMinimumHeight(15);

  _slider->SetMinimumWidth(3);
  _slider->SetMaximumWidth(3);
  SetMinimumWidth(3);
  SetMaximumWidth(3);
}

PlacesVScrollBar::~PlacesVScrollBar()
{
  if (_slider_texture)
    _slider_texture->UnReference();
}

void
PlacesVScrollBar::PreLayoutManagement()
{
  nux::VScrollBar::PreLayoutManagement();
}

long
PlacesVScrollBar::PostLayoutManagement(long LayoutResult)
{
  long ret = nux::VScrollBar::PostLayoutManagement(LayoutResult);

  UpdateTexture();
  return ret;
}

void
PlacesVScrollBar::Draw(nux::GraphicsEngine& graphics_engine, bool force_draw)
{
  if(!RedirectedAncestor())
  {  
    DrawScrollbar(graphics_engine);
  }
}

void
PlacesVScrollBar::DrawContent(nux::GraphicsEngine& graphics_engine, bool force_draw)
{
  if(RedirectedAncestor())
  {   
    DrawScrollbar(graphics_engine); 
  }
}

void
PlacesVScrollBar::DrawScrollbar(nux::GraphicsEngine& graphics_engine)
{
  nux::Color color = nux::color::White;
  nux::Geometry const& base  = GetGeometry();
  nux::TexCoordXForm texxform;

  graphics_engine.PushClippingRectangle(base);
  unsigned int alpha = 0, src = 0, dest = 0;
  graphics_engine.GetRenderStates().GetBlend(alpha, src, dest);

  // check if textures have been computed... if they haven't, exit function
  if (!_slider_texture)
    return;

  texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_SCALE_COORD);

  graphics_engine.GetRenderStates().SetBlend(true);
  graphics_engine.GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);

  if (content_height_ > container_height_)
  {
    nux::Geometry const& slider_geo = _slider->GetGeometry();

    graphics_engine.QRP_1Tex(slider_geo.x,
                        slider_geo.y,
                        slider_geo.width,
                        slider_geo.height,
                        _slider_texture->GetDeviceTexture(),
                        texxform,
                        color);
  }

  graphics_engine.PopClippingRectangle();
  graphics_engine.GetRenderStates().SetBlend(alpha, src, dest);
}

void PlacesVScrollBar::UpdateTexture()
{
  nux::CairoGraphics* cairoGraphics = NULL;
  cairo_t*            cr            = NULL;

  // update texture of slider
  int width  = _slider->GetBaseWidth();
  int height = _slider->GetBaseHeight();
  cairoGraphics = new nux::CairoGraphics(CAIRO_FORMAT_ARGB32, width, height);
  cr = cairoGraphics->GetContext();

  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 1.0f);
  cairoGraphics->DrawRoundedRectangle(cr,
                                      1.0f,
                                      0.0,
                                      0.0,
                                      1.5,
                                      3.0,
                                      (double) height - 3.0);
  cairo_fill(cr);

  if (_slider_texture)
    _slider_texture->UnReference();
  _slider_texture = texture_from_cairo_graphics(*cairoGraphics);

  cairo_destroy(cr);
  delete cairoGraphics;
}

} // namespace dash
} // namespace unity

