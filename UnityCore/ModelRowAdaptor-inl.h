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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#ifndef UNITY_MODEL_ROW_INL_H
#define UNITY_MODEL_ROW_INL_H

namespace unity
{
namespace dash
{

template<typename T>
void RowAdaptorBase::set_renderer(T renderer)
{
  set_model_tag(renderer);
}

template<typename T>
T RowAdaptorBase::renderer() const
{
  return static_cast<T>(get_model_tag());
}

}
}

#endif
