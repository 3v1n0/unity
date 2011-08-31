/*
 * Copyright 2011 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 *
 */



#ifndef RESULTRENDERERTILE_H
#define RESULTRENDERERTILE_H

#include <Nux/Nux.h>
#include <NuxCore/Object.h>
#include <NuxCore/Property.h>
#include "Nux/TextureArea.h"
#include "NuxImage/CairoGraphics.h"

#include "ResultRenderer.h"

namespace unity
{
namespace dash
{

// Initially unowned object
class ResultRendererTile : public ResultRenderer
{
public:
  ResultRendererTile(NUX_FILE_LINE_PROTO);
  virtual ~ResultRendererTile();

  inline virtual void Render(nux::GraphicsEngine& GfxContext, Result& row, ResultRendererState state, nux::Geometry& geometry, int x_offset, int y_offset);
  virtual void Preload(Result& row);  // this is just to start preloading images and text that the renderer might need - can be ignored
  virtual void Unload(Result& row);  // unload any previous grabbed images

private:
  void LoadText(std::string& text);
  void LoadIcon(std::string& icon_hint);

  //icon loading callbacks
  void IconLoaded(std::string const& texid,
                  unsigned size,
                  GdkPixbuf* pixbuf,
                  std::string const& icon_name);
  void CreateTextureCallback(const char* texid, int width, int height, nux::BaseTexture** texture, GdkPixbuf* pixbuf);
  void CreateBlurredTextureCallback(const char* texid, int width, int height, nux::BaseTexture** texture, GdkPixbuf* pixbuf);
  void DrawHighlight(const char* texid, int width, int height, nux::BaseTexture** texture);

  typedef std::map<std::string, nux::BaseTexture*> LocalTextureCache;

  LocalTextureCache icon_cache_;
  LocalTextureCache blurred_icon_cache_;
  LocalTextureCache text_cache_;
  nux::BaseTexture* prelight_cache_;
  std::map<std::string, uint> currently_loading_icons_;

};

}
}
#endif // RESULTRENDERERTILE_H
