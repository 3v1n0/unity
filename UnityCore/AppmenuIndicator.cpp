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
 * Authored by: Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 */

#include "AppmenuIndicator.h"

namespace unity
{
namespace indicator
{

struct AppmenuIndicator::Impl
{
  Impl(AppmenuIndicator* parent)
  {}
};

AppmenuIndicator::AppmenuIndicator(std::string const& name)
  : Indicator(name)
  , impl_(new AppmenuIndicator::Impl(this))
{}

AppmenuIndicator::~AppmenuIndicator()
{}

Indicator::Entries AppmenuIndicator::GetEntriesForWindow(uint32_t parent_window)
{
  Entries entries;

  for (auto const& entry : GetEntries())
  {
    if (entry->parent_window() == parent_window)
      entries.push_back(entry);
  }

  return entries;
}

void AppmenuIndicator::ShowAppmenu(unsigned int xid, int x, int y) const
{
  on_show_appmenu.emit(xid, x, y);
}

} // namespace indicator
} // namespace unity
