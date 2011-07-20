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

#include "AbstractLauncherIcon.h"
#include "AbstractIconRenderer.h"

namespace unity {
namespace ui {

class IconRenderer : public AbstractIconRenderer
{
public:
  IconRenderer();
  virtual ~IconRenderer();

  void PreprocessIcons (std::list<RenderArg>& args, nux::Geometry const& target_window);

  void RenderIcon (nux::GraphicsEngine& GfxContext, RenderArg const& arg, nux::Geometry const& anchor_geo, nux::Geometry const& owner_geo);

  void SetTargetSize (int tile_size, int image_size, int spacing);

protected:
  nux::BaseTexture* RenderCharToTexture (const char label, int width, int height);                              

  void RenderElement (nux::GraphicsEngine& GfxContext,
                      RenderArg const& arg,
                      nux::IntrusiveSP<nux::IOpenGLBaseTexture> icon,
                      nux::Color bkg_color,
                      float alpha,
                      std::vector<nux::Vector4>& xform_coords);
  
  void RenderIndicators (nux::GraphicsEngine& GfxContext,
                         RenderArg const& arg,
                         int running,
                         int active,
                         float alpha,
                         nux::Geometry const& geo);

  void RenderProgressToTexture (nux::GraphicsEngine& GfxContext, 
                                nux::IntrusiveSP<nux::IOpenGLBaseTexture> texture, 
                                float progress_fill, 
                                float bias);

  void UpdateIconTransform (AbstractLauncherIcon *icon, nux::Matrix4 ViewProjectionMatrix, nux::Geometry const& geo,
                            float x, float y, float w, float h, float z, std::string name);
  
  void UpdateIconSectionTransform (AbstractLauncherIcon *icon, nux::Matrix4 ViewProjectionMatrix, nux::Geometry const& geo,
                                   float x, float y, float w, float h, float z, float xx, float yy, float ww, float hh, std::string name);

  void GetInverseScreenPerspectiveMatrix(nux::Matrix4& ViewMatrix, nux::Matrix4& PerspectiveMatrix,
                                         int ViewportWidth,
                                         int ViewportHeight,
                                         float NearClipPlane,
                                         float FarClipPlane,
                                         float Fovy);

  void SetOffscreenRenderTarget (nux::IntrusiveSP<nux::IOpenGLBaseTexture> texture);

  void RestoreSystemRenderTarget ();

  void SetupShaders ();

  void GenerateTextures ();

  void DestroyTextures ();

private:
  enum IconSize
  {
    SMALL = 0,
    BIG,

    LAST,
  };

  int icon_size;
  int image_size;
  int spacing;

  static bool textures_created;

  static nux::BaseTexture* _progress_bar_trough;
  static nux::BaseTexture* _progress_bar_fill;
  
  static nux::BaseTexture* _pip_ltr;
  static nux::BaseTexture* _pip_rtl;
  static nux::BaseTexture* _arrow_ltr;
  static nux::BaseTexture* _arrow_rtl;
  static nux::BaseTexture* _arrow_empty_ltr;
  static nux::BaseTexture* _arrow_empty_rtl;

  static std::vector<nux::BaseTexture*> _icon_back;
  static std::vector<nux::BaseTexture*> _icon_selected_back;
  static std::vector<nux::BaseTexture*> _icon_edge;
  static std::vector<nux::BaseTexture*> _icon_glow;
  static std::vector<nux::BaseTexture*> _icon_shine;

  static nux::IntrusiveSP<nux::IOpenGLShaderProgram>    _shader_program_uv_persp_correction;
  static nux::IntrusiveSP<nux::IOpenGLAsmShaderProgram> _AsmShaderProg;

  static nux::IntrusiveSP<nux::IOpenGLBaseTexture> _offscreen_progress_texture;

  static std::map<char, nux::BaseTexture*> label_map;
};

}
}

#endif // ICONRENDERER_H

