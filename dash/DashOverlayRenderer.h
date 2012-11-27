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


#ifndef DASHOVERLAYRENDERER_H
#define DASHOVERLAYRENDERER_H

#include "unity-shared/OverlayRenderer.h"

namespace unity 
{
namespace dash
{

class DashOverlayRenderer : public OverlayRenderer
{
public:
  DashOverlayRenderer();

  nux::Property<bool> refine_open;

protected:
  virtual void CustomDrawFull(nux::GraphicsEngine& gfx_context, nux::Geometry const& content_geo, nux::Geometry const& absolute_geo, nux::Geometry const& geo, int border);
  virtual void CustomDrawInner(nux::GraphicsEngine& gfx_context, nux::Geometry const& content_geo, nux::Geometry const& absolute_geo, nux::Geometry const& geo, int border);
  virtual void CustomDrawCleanup(nux::GraphicsEngine& gfx_context, nux::Geometry const& content_geo, nux::Geometry const& absolute_geo, nux::Geometry const& geo);

private:
  void UpdateRefineTexture();

  int bgs;
  bool refine_is_open_;

  nux::ObjectPtr<nux::BaseTexture> bg_refine_tex_;
  nux::ObjectPtr<nux::BaseTexture> bg_refine_no_refine_tex_;

  std::unique_ptr<nux::TextureLayer> bg_refine_gradient_;
};

}
}
#endif // DASHOVERLAYRENDERER_H
