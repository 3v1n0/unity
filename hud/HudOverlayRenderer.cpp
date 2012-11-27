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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 */


#include "HudOverlayRenderer.h"

#include "unity-shared/DashStyle.h"

namespace unity 
{
namespace hud
{

namespace
{
const int EXCESS_BORDER = 10;
}

HudOverlayRenderer::HudOverlayRenderer()
: bgs(0)
{
  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
  nux::TexCoordXForm texxform;

  bg_right_gradient_.reset(new nux::TextureLayer(unity::dash::Style::Instance().GetRefineTextureDash()->GetDeviceTexture(), 
                            texxform, 
                            nux::color::White,
                            false,
                            rop));
}

void HudOverlayRenderer::CustomDrawFull(nux::GraphicsEngine& gfx_context, nux::Geometry const& content_geo, nux::Geometry const& absolute_geo, nux::Geometry const& geo, int border)
{
  nux::Geometry larger_content_geo = content_geo;
  larger_content_geo.OffsetSize(border, border);

  if (bg_right_gradient_)
  {
    nux::Geometry geo_refine(larger_content_geo.x + larger_content_geo.width - bg_right_gradient_->GetDeviceTexture()->GetWidth(), 
                             larger_content_geo.y,
                             bg_right_gradient_->GetDeviceTexture()->GetWidth(), 
                             larger_content_geo.height);

    bg_right_gradient_->SetGeometry(geo_refine);
    bg_right_gradient_->Renderlayer(gfx_context);
  }
}

void HudOverlayRenderer::CustomDrawInner(nux::GraphicsEngine& gfx_context, nux::Geometry const& content_geo, nux::Geometry const& absolute_geo, nux::Geometry const& geo, int border)
{
  if (bg_right_gradient_)
  {
    nux::GetPainter().PushLayer(gfx_context, bg_right_gradient_->GetGeometry(), bg_right_gradient_.get());
    bgs++;
  }
}

void HudOverlayRenderer::CustomDrawCleanup(nux::GraphicsEngine& gfx_context, nux::Geometry const& content_geo, nux::Geometry const& absolute_geo, nux::Geometry const& geo)
{
  nux::GetPainter().PopBackground(bgs);
  bgs = 0;
}

}
}
