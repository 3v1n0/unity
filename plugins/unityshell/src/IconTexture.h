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
 * Authored by: Gord Allott <gord.allott@canonical.com>
 */

#ifndef ICON_TEXTURE_H
#define ICON_TEXTURE_H

#include <Nux/TextureArea.h>
#include <Nux/View.h>
#include <NuxImage/CairoGraphics.h>
#include <NuxGraphics/GraphicsEngine.h>

#include "Introspectable.h"

namespace unity
{

class IconTexture : public nux::TextureArea, public unity::Introspectable
{
public:
  IconTexture(nux::BaseTexture* texture, guint width, guint height);
  IconTexture(const char* icon_name, unsigned int size, bool defer_icon_loading = false);
  ~IconTexture();

  void SetByIconName(const char* icon_name, unsigned int size);
  void SetByFilePath(const char* file_path, unsigned int size);
  void GetTextureSize(int* width, int* height);

  void LoadIcon();

  void SetOpacity(float opacity);
  void SetTexture(nux::BaseTexture* texture);

  void SetAcceptKeyNavFocus(bool accept);

  nux::BaseTexture* texture();

protected:
  // Key navigation
  virtual bool AcceptKeyNavFocus();
  bool _accept_key_nav_focus;

  const gchar* GetName();
  void AddProperties(GVariantBuilder* builder);
  virtual bool DoCanFocus();

protected:
  void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);

private:
  nux::BaseTexture* CreateTextureCallback(std::string const& texid, int width, int height);
  void Refresh(GdkPixbuf* pixbuf);
  void IconLoaded(std::string const& icon_name, unsigned size, GdkPixbuf* pixbuf);

  // FIXME: make _icon_name a std::string.
  char* _icon_name;
  unsigned int _size;

  GdkPixbuf*        _pixbuf_cached;
  nux::ObjectPtr<nux::BaseTexture> _texture_cached;
  // FIXME: make these two a nux::Size.
  int               _texture_width;
  int               _texture_height;

  bool _loading;

  float _opacity;
};

}

#endif // ICON_TEXTURE_H
