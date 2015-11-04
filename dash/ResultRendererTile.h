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

#include "ResultRenderer.h"
#include "unity-shared/IconLoader.h"

namespace unity
{
namespace dash
{
  struct TextureContainer
  {
    typedef nux::ObjectPtr<nux::BaseTexture> BaseTexturePtr;
    BaseTexturePtr text;
    BaseTexturePtr icon;
    BaseTexturePtr prelight;
    glib::Object<GdkPixbuf> drag_icon;

    int slot_handle;

    TextureContainer()
      : slot_handle(0)
    {}

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
  virtual ~ResultRendererTile() {}

  virtual void Render(nux::GraphicsEngine& GfxContext,
                      Result& row,
                      ResultRendererState state,
                      nux::Geometry const& geometry,
                      int x_offset, int y_offset,
                      nux::Color const& color,
                      float saturate);

  virtual void Preload(Result const& row);
  virtual void Unload(Result const& row);
  virtual nux::NBitmapData* GetDndImage(Result const& row) const;

  void ReloadResult(Result const& row);

  int Padding() const;

protected:
  virtual void LoadText(Result const& row);
  void LoadIcon(Result const& row);
  nux::ObjectPtr<nux::BaseTexture> prelight_cache_;
  nux::ObjectPtr<nux::BaseTexture> normal_cache_;
private:
  //icon loading callbacks
  void IconLoaded(std::string const& texid, int max_width, int max_height,
                  glib::Object<GdkPixbuf> const& pixbuf,
                  std::string const& icon_name, Result const& row);
  nux::BaseTexture* CreateTextureCallback(std::string const& texid,
                                          int width, int height,
                                          glib::Object<GdkPixbuf> const& pixbuf);
  nux::BaseTexture* DrawHighlight(std::string const& texid,
                                  int width, int height);

  void UpdateWidthHeight();
};

}
}

#endif // RESULTRENDERERTILE_H

