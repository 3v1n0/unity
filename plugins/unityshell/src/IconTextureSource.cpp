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

IconTextureSource::IconTextureSource()
{
  transform_map.resize(max_num_monitors);
}

std::vector<nux::Vector4> & IconTextureSource::GetTransform(TransformIndex index, int monitor)
{
  auto iter = transform_map[monitor].find(index);
  if (iter == transform_map[monitor].end())
  {
    auto iter2 = transform_map[monitor].insert(std::map<TransformIndex, std::vector<nux::Vector4> >::value_type(index, std::vector<nux::Vector4>(4)));
    return iter2.first->second;
  }

  return iter->second;
}

}
}