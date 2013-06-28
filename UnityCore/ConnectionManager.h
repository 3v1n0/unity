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

#ifndef UNITY_CONNECTION_MANAGER
#define UNITY_CONNECTION_MANAGER

#include <sigc++/sigc++.h>
#include <memory>
#include <unordered_map>

namespace unity
{
namespace connection
{
typedef uint64_t handle;

class Manager
{
public:
  Manager() = default;
  ~Manager();

  handle Add(sigc::connection const&);
  bool Remove(handle);
  handle Replace(handle, sigc::connection const&);

  bool Empty() const;
  size_t Size() const;

private:
  Manager(Manager const&) = delete;
  Manager& operator=(Manager const&) = delete;

  std::unordered_map<handle, sigc::connection> connections_;
};
}
} // unity namespace

#endif
