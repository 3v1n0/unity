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

#include "PlacesVScrollBar.h"
#include "unity-shared/CairoTexture.h"
#include "unity-shared/DashStyle.h"
#include "unity-shared/GraphicsUtils.h"

namespace unity
{
namespace dash
{

PlacesVScrollBar::PlacesVScrollBar(NUX_FILE_LINE_DECL)
  : nux::VScrollBar(NUX_FILE_LINE_PARAM)
  , scale(1.0)
  , hovering(false)
{
  Style::Instance().changed.connect(sigc::mem_fun(this, &PlacesVScrollBar::OnStyleChanged));
  scale.changed.connect([this] (double scale) {
    QueueRelayout();
    QueueDraw();
  });
}

void PlacesVScrollBar::OnStyleChanged()
{
  slider_texture_.Release();
  QueueDraw();
}

void PlacesVScrollBar::Draw(nux::GraphicsEngine& graphics_engine, bool force_draw)
{}

void PlacesVScrollBar::DrawContent(nux::GraphicsEngine& graphics_engine, bool force_draw)
{
  if (!RedirectedAncestor())
    return;

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
    nux::GetPainter().PushBackgroundStack();

    if (hovering)
      graphics::ClearGeometry(base);

    nux::ROPConfig rop;
    rop.Blend = true;
    rop.SrcBlend = GL_ONE;
    rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

    nux::ColorLayer layer(nux::color::Transparent, true, rop);
    nux::GetPainter().PushDrawLayer(graphics_engine, base, &layer);

    if (hovering)
    {
      auto const& track_color = Style::Instance().GetScrollbarTrackColor();
      graphics_engine.QRP_Color(base.x, base.y, base.width, base.height, track_color * track_color.alpha);
    }

    UpdateTexture(slider_geo);
    graphics_engine.QRP_1Tex(base.x + base.width - slider_geo.width,
                             slider_geo.y,
                             slider_geo.width,
                             slider_geo.height,
                             slider_texture_->GetDeviceTexture(),
                             texxform,
                             nux::color::White);

    nux::GetPainter().PopBackgroundStack();
  }

  graphics_engine.PopClippingRectangle();
  graphics_engine.GetRenderStates().SetBlend(alpha, src, dest);
}

void PlacesVScrollBar::UpdateTexture(nux::Geometry const& geo)
{
  // update texture of slider
  int width  = geo.width;
  int height = geo.height;

  if (slider_texture_ && slider_texture_->GetWidth() == width && slider_texture_->GetHeight() == height)
    return;

  auto& style = Style::Instance();
  double unscaled_width = static_cast<double>(width) / scale();
  double unscaled_height = static_cast<double>(height) / scale();

  nux::CairoGraphics cg(CAIRO_FORMAT_ARGB32, width, height);
  auto* cr = cg.GetInternalContext();
  cairo_surface_set_device_scale(cairo_get_target(cr), scale, scale);

  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  auto const& color = hovering ? style.GetScrollbarColor() : style.GetOverlayScrollbarColor();
  double radius = hovering ? style.GetScrollbarCornerRadius() : style.GetOverlayScrollbarCornerRadius();

  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_rgba(cr, color.red, color.green, color.blue, color.alpha);
  cg.DrawRoundedRectangle(cr, 1.0f, 0, 0, radius, unscaled_width, unscaled_height - 2.0);
  cairo_fill(cr);

  slider_texture_ = texture_ptr_from_cairo_graphics(cg);
}

} // namespace dash
} // namespace unity

