// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011-2014 Canonical Ltd
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

#include <Nux/Nux.h>
#include <NuxGraphics/CairoGraphics.h>

#include "unity-shared/CairoTexture.h"
#include "unity-shared/RawPixel.h"
#include "PlacesVScrollBar.h"

namespace unity
{
namespace dash
{
namespace
{
const RawPixel BUTTONS_HEIGHT = 15_em;
const RawPixel WIDTH = 3_em;
}

PlacesVScrollBar::PlacesVScrollBar(NUX_FILE_LINE_DECL)
  : nux::VScrollBar(NUX_FILE_LINE_PARAM)
  , scale(1.0)
{
  UpdateSize();
  scale.changed.connect([this] (double scale) {
    UpdateSize();
    QueueRelayout();
    QueueDraw();
  });
}

void PlacesVScrollBar::UpdateSize()
{
  _scroll_up_button->SetMaximumHeight(BUTTONS_HEIGHT.CP(scale));
  _scroll_up_button->SetMinimumHeight(BUTTONS_HEIGHT.CP(scale));

  _scroll_down_button->SetMaximumHeight(BUTTONS_HEIGHT.CP(scale));
  _scroll_down_button->SetMinimumHeight(BUTTONS_HEIGHT.CP(scale));

  _slider->SetMinimumWidth(WIDTH.CP(scale));
  _slider->SetMaximumWidth(WIDTH.CP(scale));
  SetMinimumWidth(WIDTH.CP(scale));
  SetMaximumWidth(WIDTH.CP(scale));
}

void PlacesVScrollBar::PreLayoutManagement()
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
  if (!RedirectedAncestor())
  {
    DrawScrollbar(graphics_engine);
  }
}

void
PlacesVScrollBar::DrawContent(nux::GraphicsEngine& graphics_engine, bool force_draw)
{
  if (RedirectedAncestor())
  {
    DrawScrollbar(graphics_engine);
  }
}

void
PlacesVScrollBar::DrawScrollbar(nux::GraphicsEngine& graphics_engine)
{
  // check if textures have been computed... if they haven't, exit function
  if (!slider_texture_)
    return;

  nux::Color color = nux::color::White;
  nux::Geometry const& base  = GetGeometry();
  nux::TexCoordXForm texxform;

  graphics_engine.PushClippingRectangle(base);
  unsigned int alpha = 0, src = 0, dest = 0;
  graphics_engine.GetRenderStates().GetBlend(alpha, src, dest);


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
                        slider_texture_->GetDeviceTexture(),
                        texxform,
                        color);
  }

  graphics_engine.PopClippingRectangle();
  graphics_engine.GetRenderStates().SetBlend(alpha, src, dest);
}

void PlacesVScrollBar::UpdateTexture()
{
  // update texture of slider
  int width  = _slider->GetBaseWidth();
  int height = _slider->GetBaseHeight();

  if (slider_texture_ && slider_texture_->GetWidth() == width && slider_texture_->GetHeight() == height)
    return;

  nux::CairoGraphics cg(CAIRO_FORMAT_ARGB32, width, height);
  auto* cr = cg.GetContext();
  cairo_surface_set_device_scale(cairo_get_target(cr), scale, scale);

  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 1.0f);
  cg.DrawRoundedRectangle(cr, 1.0f, 0.0, 0.0, 1.5, 3.0, static_cast<double>(height)/scale() - 3.0);
  cairo_fill(cr);

  slider_texture_ = texture_ptr_from_cairo_graphics(cg);
}

} // namespace dash
} // namespace unity

