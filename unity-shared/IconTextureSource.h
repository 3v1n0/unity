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
 *
 */

#ifndef ICONTEXTURESOURCE_H
#define ICONTEXTURESOURCE_H

#include <Nux/Nux.h>
#include <NuxCore/Property.h>
#include <NuxCore/Math/MathInc.h>

namespace unity
{
namespace ui
{

class IconTextureSource : public nux::InitiallyUnownedObject
{
  NUX_DECLARE_OBJECT_TYPE(IconTextureSource, nux::InitiallyUnownedObject);
public:
  typedef nux::ObjectPtr<IconTextureSource> Ptr;

  enum TransformIndex
  {
    TRANSFORM_TILE,
    TRANSFORM_IMAGE,
    TRANSFORM_HIT_AREA,
    TRANSFORM_GLOW,
    TRANSFORM_EMBLEM,
  };

  IconTextureSource();

  std::vector<nux::Vector4> & GetTransform(TransformIndex index, int monitor);

  virtual nux::Color BackgroundColor() const = 0;

  virtual nux::Color GlowColor() = 0;

  virtual nux::BaseTexture* TextureForSize(int size) = 0;

  virtual nux::BaseTexture* Emblem() = 0;

private:
  std::vector<std::map<TransformIndex, std::vector<nux::Vector4> > > transform_map;
};

}
}

#endif // LAUNCHERICON_H

