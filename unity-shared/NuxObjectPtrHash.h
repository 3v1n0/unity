// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2015 Canonical Ltd
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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#ifndef NUX_OBJECT_PTR_HASH
#define NUX_OBJECT_PTR_HASH

#include <functional>
namespace std
{
// Template specialization, needed for unordered_{map,set}
template<typename T> struct hash<nux::ObjectPtr<T>>
{
  std::size_t operator()(nux::ObjectPtr<T> const& o) const
  {
    return std::hash<T*>()(o.GetPointer());
  }
};
}

#endif // NUX_OBJECT_PTR_HASH
