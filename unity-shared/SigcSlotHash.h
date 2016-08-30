// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2016 Canonical Ltd
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

#ifndef __UNITY_SIGC_SLOT_HASHER__
#define __UNITY_SIGC_SLOT_HASHER__

#include <sigc++/slot.h>

namespace sigc
{
inline bool operator==(slot_base const& lhs, slot_base const& rhs)
{
  if (!lhs.rep_ || !rhs.rep_)
    return (lhs.rep_ == rhs.rep_);

  return (lhs.rep_->call_ == rhs.rep_->call_);
}

inline bool operator!=(slot_base const& lhs, slot_base const& rhs)
{
  return !(lhs == rhs);
}
} // sigc namespace

namespace std
{

template<>
struct hash<sigc::slot_base>
{
  size_t operator()(sigc::slot_base const& cb) const
  {
    if (cb.rep_)
      return hash<size_t>()(reinterpret_cast<size_t>(cb.rep_->call_));

    return hash<size_t>()(reinterpret_cast<size_t>(cb.rep_));
  }
};

#if __GNUC__ < 6
template<class T>
struct hash
{
  size_t operator()(T const& cb) const
  {
    static_assert(std::is_base_of<sigc::slot_base, T>::value, "Type is not derived from sigc::slot_base");
    return hash<sigc::slot_base>()(cb);
  }
};
#endif

} // std namespace

#endif // __UNITY_SIGC_SLOT_HASHER__
