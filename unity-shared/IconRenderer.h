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
 * Authored by: Jason Smith <jason.smith@canonical.com>
 */

#ifndef ICONRENDERER_H
#define ICONRENDERER_H

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>
#include <Nux/View.h>

#include "AbstractIconRenderer.h"

namespace unity
{
namespace ui
{

class IconTextureSource;

class IconRenderer : public AbstractIconRenderer
{
public:
  IconRenderer();

  void PreprocessIcons(std::list<RenderArg>& args, nux::Geometry const& target_window);

  void RenderIcon(nux::GraphicsEngine& GfxContext, RenderArg const& arg, nux::Geometry const& anchor_geo, nux::Geometry const& owner_geo);

  void SetTargetSize(int tile_size, int image_size, int spacing);

  static void DestroyShortcutTextures();

protected:
  void RenderElement(nux::GraphicsEngine& GfxContext,
                     RenderArg const& arg,
                     nux::ObjectPtr<nux::IOpenGLBaseTexture> const& icon,
                     nux::Color const& bkg_color,
                     nux::Color const& colorify,
                     float alpha,
                     bool force_filter,
                     std::vector<nux::Vector4> const& xform_coords);

  void RenderIndicators(nux::GraphicsEngine& GfxContext,
                        RenderArg const& arg,
                        int running,
                        int active,
                        float alpha,
                        nux::Geometry const& geo);

  void RenderProgressToTexture(nux::GraphicsEngine& GfxContext,
                               nux::ObjectPtr<nux::IOpenGLBaseTexture> const& texture,
                               float progress_fill,
                               float bias);

  void UpdateIconTransform(ui::IconTextureSource* icon, nux::Matrix4 const& ViewProjectionMatrix, nux::Geometry const& geo,
                           float x, float y, float w, float h, float z, ui::IconTextureSource::TransformIndex index);

  void UpdateIconSectionTransform(ui::IconTextureSource* icon, nux::Matrix4 const& ViewProjectionMatrix, nux::Geometry const& geo,
                                  float x, float y, float w, float h, float z, float xx, float yy, float ww, float hh, ui::IconTextureSource::TransformIndex index);

  void GetInverseScreenPerspectiveMatrix(nux::Matrix4& ViewMatrix, nux::Matrix4& PerspectiveMatrix,
                                         int ViewportWidth,
                                         int ViewportHeight,
                                         float NearClipPlane,
                                         float FarClipPlane,
                                         float Fovy);

private:
  int icon_size;
  int image_size;
  int spacing;

  struct TexturesPool;
  std::shared_ptr<TexturesPool> textures_;
  nux::Matrix4 stored_projection_matrix_;
};

}
}

#endif // ICONRENDERER_H

