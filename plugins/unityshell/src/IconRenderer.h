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

#define MAX_SHORTCUT_LABELS 10

class IconRenderer : public AbstractIconRenderer
{
public:
  IconRenderer(int icon_size);
  virtual ~IconRenderer();

  void PreprocessIcons (std::list<RenderArg> &args, nux::Geometry target_window);

  void RenderIcon (nux::GraphicsEngine& GfxContext, RenderArg const &arg, nux::Geometry anchor_geo);

  void SetTargetSize (int size);

  nux::BaseTexture* RenderCharToTexture (const char label,
                                         int        width,
                                         int        height);                              

protected:
  void RenderElement (nux::GraphicsEngine& GfxContext,
                      RenderArg const &arg,
                      nux::IntrusiveSP<nux::IOpenGLBaseTexture> icon,
                      nux::Color bkg_color,
                      float alpha,
                      nux::Vector4 xform_coords[]);
  
  void RenderIndicators (nux::GraphicsEngine& GfxContext,
                         RenderArg const &arg,
                         int running,
                         int active,
                         float alpha,
                         nux::Geometry& geo);

  void RenderProgressToTexture (nux::GraphicsEngine& GfxContext, 
                                nux::IntrusiveSP<nux::IOpenGLBaseTexture> texture, 
                                float progress_fill, 
                                float bias);

private:
  void GenerateTextures ();
  void DestroyTextures ();

  int icon_size;

  nux::BaseTexture* _icon_bkg_texture;
  nux::BaseTexture* _icon_shine_texture;
  nux::BaseTexture* _icon_outline_texture;
  nux::BaseTexture* _icon_glow_texture;
  nux::BaseTexture* _icon_glow_hl_texture;
  nux::BaseTexture* _progress_bar_trough;
  nux::BaseTexture* _progress_bar_fill;
  
  nux::BaseTexture* _pip_ltr;
  nux::BaseTexture* _pip_rtl;
  nux::BaseTexture* _arrow_ltr;
  nux::BaseTexture* _arrow_rtl;
  nux::BaseTexture* _arrow_empty_ltr;
  nux::BaseTexture* _arrow_empty_rtl;

  nux::BaseTexture* _superkey_labels[MAX_SHORTCUT_LABELS];
};

}
}

#endif // ICONRENDERER_H

