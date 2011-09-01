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
#include "IconLoader.h"

#include "ResultRenderer.h"

namespace unity
{
namespace dash
{
  struct TextureContainer
  {
    nux::BaseTexture* text;
    nux::BaseTexture* icon;
    nux::BaseTexture* blurred_icon;
    int slot_handle;

    TextureContainer()
      : text(NULL)
      , icon(NULL)
      , blurred_icon(NULL)
      , slot_handle(0)
    {
    }

    ~TextureContainer()
    {
      if (text != NULL)
      {
        text->UnReference();
        if (text->GetReferenceCount() == 1) // because textures are made with an extra reference for bad reasons, we have to do this. its bad.
          text->UnReference();
      }

      if (icon != NULL)
      {
        icon->UnReference();
        if (icon->GetReferenceCount() == 1)
          icon->UnReference();
      }
      if (blurred_icon != NULL)
      {
        blurred_icon->UnReference();
        if (blurred_icon->GetReferenceCount() == 1)
          blurred_icon->UnReference();
      }
      if (slot_handle > 0)
        IconLoader::GetDefault()->DisconnectHandle(slot_handle);
    }
  };

  typedef TextureContainer TextureContainer;

// Initially unowned object
class ResultRendererTile : public ResultRenderer
{
public:
  ResultRendererTile(NUX_FILE_LINE_PROTO);
  virtual ~ResultRendererTile();

  inline virtual void Render(nux::GraphicsEngine& GfxContext, Result& row, ResultRendererState state, nux::Geometry& geometry, int x_offset, int y_offset);
  virtual void Preload(Result& row);  // this is just to start preloading images and text that the renderer might need - can be ignored
  virtual void Unload(Result& row);  // unload any previous grabbed images

protected:
  void LoadText(Result& row);
  void LoadIcon(std::string& icon_hint, Result& row);
  nux::BaseTexture* prelight_cache_;

private:
  //icon loading callbacks
  void IconLoaded(const char* texid, guint size, GdkPixbuf* pixbuf, std::string icon_name, Result& row);
  void CreateTextureCallback(const char* texid, int width, int height, nux::BaseTexture** texture, GdkPixbuf* pixbuf);
  void CreateBlurredTextureCallback(const char* texid, int width, int height, nux::BaseTexture** texture, GdkPixbuf* pixbuf);
  void DrawHighlight(const char* texid, int width, int height, nux::BaseTexture** texture);


};

}
}
#endif // RESULTRENDERERTILE_H
