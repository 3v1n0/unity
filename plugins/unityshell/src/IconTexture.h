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

#include <Nux/Nux.h>
// FIXME: Nux/View.h needs Nux.h included first.
#include <Nux/View.h>
// FIXME: Nux/TextureArea.h needs View included first.
#include <Nux/TextureArea.h>
#include <NuxImage/CairoGraphics.h>
#include <NuxGraphics/GraphicsEngine.h>

#include "Introspectable.h"

namespace unity
{

class IconTexture : public nux::TextureArea, public unity::debug::Introspectable
{
public:
  IconTexture(nux::BaseTexture* texture, guint width, guint height);
  IconTexture(std::string const& icon_name, unsigned int size, bool defer_icon_loading = false);
  virtual ~IconTexture();

  void SetByIconName(std::string const& icon_name, unsigned int size);
  void SetByFilePath(std::string const& file_path, unsigned int size);
  void GetTextureSize(int* width, int* height);

  void LoadIcon();

  void SetOpacity(float opacity);
  void SetTexture(nux::BaseTexture* texture);

  void SetAcceptKeyNavFocus(bool accept);

  nux::BaseTexture* texture();

  sigc::signal<void, nux::BaseTexture*> texture_updated;

protected:
  // Key navigation
  virtual bool AcceptKeyNavFocus();
  bool _accept_key_nav_focus;

  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);
  virtual bool DoCanFocus();
  GdkPixbuf* _pixbuf_cached;

protected:
  void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);

private:
  nux::BaseTexture* CreateTextureCallback(std::string const& texid, int width, int height);
  void Refresh(GdkPixbuf* pixbuf);
  void IconLoaded(std::string const& icon_name, unsigned size, GdkPixbuf* pixbuf);

  std::string _icon_name;
  unsigned int _size;

  nux::ObjectPtr<nux::BaseTexture> _texture_cached;
  // FIXME: make these two a nux::Size.
  int               _texture_width;
  int               _texture_height;

  bool _loading;

  float _opacity;
  int _handle;
};

}

#endif // ICON_TEXTURE_H
