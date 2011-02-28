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

class IconTexture : public nux::TextureArea, public Introspectable
{
public:
  IconTexture (const char *icon_name, unsigned int size, bool defer_icon_loading=false);
  ~IconTexture ();

  void SetByIconName (const char *icon_name, unsigned int size);
  void SetByFilePath (const char *file_path, unsigned int size);
  void GetTextureSize (int *width, int *height);
  
  void LoadIcon ();

protected:
  const gchar* GetName ();
  void AddProperties (GVariantBuilder *builder);

private:
  void Draw (nux::GraphicsEngine& GfxContext, bool force_draw);

  void CreateTextureCallback (const char *texid, int width, int height, nux::BaseTexture **texture);
  void Refresh (GdkPixbuf *pixbuf);
  void IconLoaded (const char *icon_name, guint size, GdkPixbuf *pixbuf);

  char *_icon_name;
  unsigned int _size;

  virtual bool CanFocus ();

  GdkPixbuf        *_pixbuf_cached;
  nux::BaseTexture *_texture_cached;
  int               _texture_width;
  int               _texture_height;

  bool _loading;
};

#endif // ICON_TEXTURE_H
