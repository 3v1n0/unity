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

#include "CairoTexture.h"
#include "PlacesVScrollBar.h"

using unity::texture_from_cairo_graphics;

const int PLACES_VSCROLLBAR_WIDTH  = 10;
const int PLACES_VSCROLLBAR_HEIGHT = 10;
const int BLUR_SIZE                =  7;

PlacesVScrollBar::PlacesVScrollBar(NUX_FILE_LINE_DECL)
  : VScrollBar(NUX_FILE_LINE_PARAM),
    _slider_texture(NULL),
    _track_texture(NULL)
{
  _slider->SetMinimumSize(PLACES_VSCROLLBAR_WIDTH + 2 * BLUR_SIZE,
                          PLACES_VSCROLLBAR_HEIGHT + 2 * BLUR_SIZE);
  _track->SetMinimumSize(PLACES_VSCROLLBAR_WIDTH + 2 * BLUR_SIZE,
                         PLACES_VSCROLLBAR_HEIGHT + 2 * BLUR_SIZE);
  SetMinimumSize(PLACES_VSCROLLBAR_WIDTH + 2 * BLUR_SIZE,
                 PLACES_VSCROLLBAR_HEIGHT + 2 * BLUR_SIZE);
}

PlacesVScrollBar::~PlacesVScrollBar()
{
  if (_slider_texture)
    _slider_texture->UnReference();

  if (_track_texture)
    _track_texture->UnReference();
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
PlacesVScrollBar::Draw(nux::GraphicsEngine& gfxContext, bool force_draw)
{
  nux::Color         color = nux::color::White;
  nux::Geometry      base  = GetGeometry();
  nux::TexCoordXForm texxform;

  gfxContext.PushClippingRectangle(base);

  nux::GetPainter().PaintBackground(gfxContext, base);

  // check if textures have been computed... if they haven't, exit function
  if (!_slider_texture)
    return;

  //texxform.SetWrap (nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
  texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_SCALE_COORD);

  gfxContext.GetRenderStates().SetBlend(true);
  gfxContext.GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);

  //base.OffsetPosition (0, PLACES_VSCROLLBAR_HEIGHT);
  //base.OffsetSize (0, -2 * PLACES_VSCROLLBAR_HEIGHT);

  if (content_height_ > container_height_)
  {
    nux::Geometry track_geo = _track->GetGeometry();
    gfxContext.QRP_1Tex(track_geo.x,
                        track_geo.y,
                        track_geo.width,
                        track_geo.height,
                        _track_texture->GetDeviceTexture(),
                        texxform,
                        color);

    nux::Geometry slider_geo = _slider->GetGeometry();
    gfxContext.QRP_1Tex(slider_geo.x - BLUR_SIZE - 2,
                        slider_geo.y,
                        slider_geo.width,
                        slider_geo.height,
                        _slider_texture->GetDeviceTexture(),
                        texxform,
                        color);
  }

  gfxContext.GetRenderStates().SetBlend(false);
  gfxContext.PopClippingRectangle();
}

void PlacesVScrollBar::UpdateTexture()
{
  nux::CairoGraphics* cairoGraphics = NULL;
  cairo_t*            cr            = NULL;

  // update texture of slider
  int width  = _slider->GetBaseWidth();
  int height = _slider->GetBaseHeight();
  cairoGraphics = new nux::CairoGraphics(CAIRO_FORMAT_ARGB32, width, height);
  width  -= 2 * BLUR_SIZE;
  height -= 2 * BLUR_SIZE;
  cr = cairoGraphics->GetContext();

  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  cairo_set_line_width(cr, 1.0f);
  cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 1.0f);
  cairoGraphics->DrawRoundedRectangle(cr,
                                      1.0f,
                                      BLUR_SIZE + 1.5f,
                                      BLUR_SIZE + 1.5f,
                                      (double)(width - 1) / 2.0f,
                                      (double) width - 3.0f,
                                      (double) height - 3.0f);
  cairo_fill_preserve(cr);
  cairoGraphics->BlurSurface(BLUR_SIZE - 3);
  cairo_fill(cr);

  if (_slider_texture)
    _slider_texture->UnReference();
  _slider_texture = texture_from_cairo_graphics(*cairoGraphics);

  cairo_destroy(cr);
  delete cairoGraphics;

  // update texture of track
  width  = _track->GetBaseWidth();
  height = _track->GetBaseHeight();
  cairoGraphics = new nux::CairoGraphics(CAIRO_FORMAT_ARGB32, width, height);
  width  -= 2 * BLUR_SIZE;
  height -= 2 * BLUR_SIZE;
  cr = cairoGraphics->GetContext();

  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  cairo_set_line_width(cr, 1.0f);
  cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 0.5f);
  cairoGraphics->DrawRoundedRectangle(cr,
                                      1.0f,
                                      BLUR_SIZE + 0.5f,
                                      BLUR_SIZE + 0.5f,
                                      (double) width / 2.0f,
                                      (double) width - 1.0f,
                                      (double) height - 1.0f);
  cairo_fill_preserve(cr);
  cairoGraphics->BlurSurface(BLUR_SIZE - 3);
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_fill_preserve(cr);
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 0.25f);
  cairo_fill_preserve(cr);
  cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 0.35f);
  cairo_stroke(cr);

  if (_track_texture)
    _track_texture->UnReference();
  _track_texture = texture_from_cairo_graphics(*cairoGraphics);

  cairo_destroy(cr);
  delete cairoGraphics;
}
