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
    TRANSFORM_TILE = 0,
    TRANSFORM_IMAGE,
    TRANSFORM_HIT_AREA,
    TRANSFORM_GLOW,
    TRANSFORM_EMBLEM,
    TRANSFORM_SIZE
  };

  IconTextureSource();

  std::vector<nux::Vector4> & GetTransform(TransformIndex index, int monitor);

  nux::Point3 const& LastRenderCenter(int monitor) const;
  nux::Point3 const& LastLogicalCenter(int monitor) const;
  nux::Vector3 const& LastRotation(int monitor) const;
  void RememberCenters(int monitor, nux::Point3 const& render, nux::Point3 const& logical);
  void RememberRotation(int monitor, nux::Vector3 const& rotation);

  void RememberSkip(int monitor, bool skip);
  bool WasSkipping(int monitor) const;

  void RememberEmblem(bool has_emblem);
  bool HadEmblem() const;

  virtual nux::Color BackgroundColor() const = 0;

  virtual nux::Color GlowColor() = 0;

  virtual nux::BaseTexture* TextureForSize(int size) = 0;

  virtual nux::BaseTexture* Emblem() = 0;

private:
  bool had_emblem_;
  std::vector<bool> skip_;
  std::vector<nux::Point3> last_render_center_;
  std::vector<nux::Point3> last_logical_center_;
  std::vector<nux::Vector3> last_rotation_;
  std::vector<std::vector<std::vector<nux::Vector4>>> transformations_;
};

}
}

#endif // LAUNCHERICON_H

