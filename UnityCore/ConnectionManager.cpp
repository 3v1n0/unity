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

#include "ConnectionManager.h"

namespace unity
{
namespace connection
{
namespace
{
handle GLOBAL_HANDLE = 0;
}

void Manager::Clear()
{
  connections_.clear();
}

handle Manager::Add(sigc::connection const& conn)
{
  handle id = 0;

  if (!conn.empty())
  {
    id = ++GLOBAL_HANDLE;
    connections_.insert({id, std::make_shared<Wrapper>(conn)});
  }

  return id;
}

bool Manager::Remove(handle id)
{
  return RemoveAndClear(&id);
}

bool Manager::RemoveAndClear(handle *id)
{
  if (*id != 0 && connections_.erase(*id))
  {
    *id = 0;
    return true;
  }

  return false;
}

handle Manager::Replace(handle const& id, sigc::connection const& conn)
{
  if (Remove(id) && !conn.empty())
  {
    connections_.insert({id, std::make_shared<Wrapper>(conn)});
    return id;
  }

  return Add(conn);
}

sigc::connection Manager::Get(handle const& id) const
{
  if (id > 0)
  {
    auto it = connections_.find(id);

    if (it != connections_.end())
      return *(it->second);
  }

  return sigc::connection();
}

bool Manager::Empty() const
{
  return connections_.empty();
}

size_t Manager::Size() const
{
  return connections_.size();
}

} // connection namespace
} // unity namespace
