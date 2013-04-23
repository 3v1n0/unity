// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012 Canonical Ltd
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
 */

#include "IconTextureSource.h"
#include "MultiMonitor.h"

namespace unity
{
namespace ui
{
NUX_IMPLEMENT_OBJECT_TYPE(IconTextureSource);

namespace
{
  const unsigned RENDERERS_SIZE = max_num_monitors + 1; // +1 for the switcher
}

IconTextureSource::IconTextureSource()
  : had_emblem_(false)
  , skip_(RENDERERS_SIZE, false)
  , last_render_center_(RENDERERS_SIZE)
  , last_logical_center_(RENDERERS_SIZE)
  , last_rotation_(RENDERERS_SIZE)
  , transformations_(RENDERERS_SIZE, decltype(transformations_)::value_type(TRANSFORM_SIZE, std::vector<nux::Vector4>(4)))
{}

std::vector<nux::Vector4> & IconTextureSource::GetTransform(TransformIndex index, int monitor)
{
  return transformations_[monitor][index];
}

void IconTextureSource::RememberCenters(int monitor, nux::Point3 const& render, nux::Point3 const& logical)
{
  last_render_center_[monitor] = render;
  last_logical_center_[monitor] = logical;
}

void IconTextureSource::RememberRotation(int monitor, nux::Vector3 const& rotation)
{
  last_rotation_[monitor] = rotation;
}

nux::Point3 const& IconTextureSource::LastRenderCenter(int monitor) const
{
  return last_render_center_[monitor];
}

nux::Point3 const& IconTextureSource::LastLogicalCenter(int monitor) const
{
  return last_logical_center_[monitor];
}

nux::Vector3 const& IconTextureSource::LastRotation(int monitor) const
{
  return last_rotation_[monitor];
}

void IconTextureSource::RememberSkip(int monitor, bool skip)
{
  skip_[monitor] = skip;
}

bool IconTextureSource::WasSkipping(int monitor) const
{
  return skip_[monitor];
}

void IconTextureSource::RememberEmblem(bool has_emblem)
{
  had_emblem_ = has_emblem;
}

bool IconTextureSource::HadEmblem() const
{
  return had_emblem_;
}

}
}