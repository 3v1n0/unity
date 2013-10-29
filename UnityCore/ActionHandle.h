// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2013 Canonical Ltd
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

#ifndef UNITY_ACTION_HANDLE_H
#define UNITY_ACTION_HANDLE_H

#include <memory>

namespace unity
{
namespace action
{
struct handle
{
  constexpr handle() : handle_(0) {}
  constexpr handle(uint64_t val) : handle_(val) {}
  constexpr operator uint64_t() const { return handle_; }
  inline handle& operator++() { ++handle_; return *this; }
  inline handle operator++(int) { auto tmp = *this; ++handle_; return tmp; }
  inline handle& operator--() { --handle_; return *this; }
  inline handle operator--(int) { auto tmp = *this; --handle_; return tmp; }

private:
  uint64_t handle_;
};
} // action namespace
} // unity namespace

namespace std
{
// Template specialization, needed for unordered_{map,set}
template<> struct hash<unity::action::handle>
{
  std::size_t operator()(unity::action::handle const& h) const
  {
    return std::hash<uint64_t>()(h);
  }
};
}

#endif // UNITY_ACTION_HANDLE_H
