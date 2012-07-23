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
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 */


#ifndef HUDICONTEXTURESOURCE_H
#define HUDICONTEXTURESOURCE_H

#include "unity-shared/IconTextureSource.h"

namespace unity
{
namespace hud
{

class HudIconTextureSource : public unity::ui::IconTextureSource
{
public:
  HudIconTextureSource(nux::ObjectPtr<nux::BaseTexture> texture);
  ~HudIconTextureSource();
  
  virtual nux::Color BackgroundColor() const;
  virtual nux::BaseTexture* TextureForSize(int size);
  virtual nux::Color GlowColor();
  virtual nux::BaseTexture* Emblem();
  void ColorForIcon(GdkPixbuf* pixbuf);
  
private:
  nux::Color bg_color;
  nux::ObjectPtr<nux::BaseTexture> icon_texture_;
};

}
}

#endif // HUDICONTEXTURESOURCE_H
