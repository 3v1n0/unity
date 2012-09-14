// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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
#include <NuxCore/ObjectPtr.h>
#include <NuxCore/Property.h>
#include <Nux/TextureArea.h>
#include <NuxGraphics/CairoGraphics.h>
#include "unity-shared/IconLoader.h"

#include "ResultRenderer.h"

namespace unity
{
namespace dash
{
  struct TextureContainer
  {
    typedef nux::ObjectPtr<nux::BaseTexture> BaseTexturePtr;
    BaseTexturePtr text;
    BaseTexturePtr icon;
    BaseTexturePtr blurred_icon;
    BaseTexturePtr prelight;
    int slot_handle;

    TextureContainer()
      : slot_handle(0)
    {
    }

    ~TextureContainer()
    {
      if (slot_handle > 0)
        IconLoader::GetDefault().DisconnectHandle(slot_handle);
    }
  };


class ResultRendererTile : public ResultRenderer
{
public:
  NUX_DECLARE_OBJECT_TYPE(ResultRendererTile, ResultRenderer);

  ResultRendererTile(NUX_FILE_LINE_PROTO);
  ~ResultRendererTile();

  virtual void Render(nux::GraphicsEngine& GfxContext,
                      Result& row,
                      ResultRendererState state,
                      nux::Geometry& geometry,
                      int x_offset, int y_offset);
  // this is just to start preloading images and text that the renderer might
  // need - can be ignored
  virtual void Preload(Result& row);
  // unload any previous grabbed images
  virtual void Unload(Result& row);

  int spacing;
  int padding;

protected:
  virtual void LoadText(Result& row);
  void LoadIcon(Result& row);
  nux::ObjectPtr<nux::BaseTexture> prelight_cache_;
  nux::ObjectPtr<nux::BaseTexture> normal_cache_;
private:
  //icon loading callbacks
  void IconLoaded(std::string const& texid, int max_width, int max_height,
                  glib::Object<GdkPixbuf> const& pixbuf,
                  std::string icon_name, Result& row);
  nux::BaseTexture* CreateTextureCallback(std::string const& texid,
                                          int width, int height,
                                          glib::Object<GdkPixbuf> const& pixbuf);
  nux::BaseTexture* DrawHighlight(std::string const& texid,
                                  int width, int height);
};

}
}
#endif // RESULTRENDERERTILE_H
