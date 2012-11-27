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


#include "DashOverlayRenderer.h"

#include "unity-shared/UnitySettings.h"
#include "unity-shared/DashStyle.h"


namespace unity 
{
namespace dash
{

namespace
{

const int EXCESS_BORDER = 10;
}

DashOverlayRenderer::DashOverlayRenderer()
: refine_open(false)
, bgs(0)
{
  bg_refine_tex_ = unity::dash::Style::Instance().GetRefineTextureDash();
  bg_refine_no_refine_tex_ = unity::dash::Style::Instance().GetRefineNoRefineTextureDash();

  refine_open.changed.connect([&](bool) { UpdateRefineTexture(); });
  UpdateRefineTexture();
}

void DashOverlayRenderer::UpdateRefineTexture()
{
  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

  nux::TexCoordXForm texxform;
  if (refine_open())
  {
    bg_refine_gradient_.reset(new nux::TextureLayer(bg_refine_tex_->GetDeviceTexture(), 
                              texxform, 
                              nux::color::White,
                              false,
                              rop));
  }
  else
  {
    bg_refine_gradient_.reset(new nux::TextureLayer(bg_refine_no_refine_tex_->GetDeviceTexture(),
                              texxform,
                              nux::color::White,
                              false,
                              rop));
  }
  need_redraw.emit();
}

void DashOverlayRenderer::CustomDrawFull(nux::GraphicsEngine& gfx_context, nux::Geometry const& content_geo, nux::Geometry const& absolute_geo, nux::Geometry const& geo, int border)
{
  nux::Geometry larger_content_geo = content_geo;
  larger_content_geo.OffsetSize(border, border);

  if (bg_refine_gradient_)
  {
    nux::Geometry geo_refine(larger_content_geo.x + larger_content_geo.width - bg_refine_gradient_->GetDeviceTexture()->GetWidth(), 
                             larger_content_geo.y,
                             bg_refine_gradient_->GetDeviceTexture()->GetWidth(), 
                             std::min(bg_refine_gradient_->GetDeviceTexture()->GetHeight(), larger_content_geo.height));

    bg_refine_gradient_->SetGeometry(geo_refine);
    bg_refine_gradient_->Renderlayer(gfx_context);
  }
}

void DashOverlayRenderer::CustomDrawInner(nux::GraphicsEngine& gfx_context, nux::Geometry const& content_geo, nux::Geometry const& absolute_geo, nux::Geometry const& geo, int border)
{
  if (bg_refine_gradient_)
  {
    nux::GetPainter().PushLayer(gfx_context, bg_refine_gradient_->GetGeometry(), bg_refine_gradient_.get());
    bgs++;
  }
}

void DashOverlayRenderer::CustomDrawCleanup(nux::GraphicsEngine& gfx_context, nux::Geometry const& content_geo, nux::Geometry const& absolute_geo, nux::Geometry const& geo)
{
  nux::GetPainter().PopBackground(bgs);
  bgs = 0;
}

}
}
