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

#include <unordered_set>

#include "AppmenuIndicator.h"
#include "ConnectionManager.h"

namespace unity
{
namespace indicator
{
namespace
{
const Indicator::Entries empty_entries_;
}

struct AppmenuIndicator::Impl
{
  Impl(AppmenuIndicator* parent)
  {
    connections_.Add(parent->on_entry_added.connect([this] (Entry::Ptr const& entry) {
      window_entries_[entry->parent_window()].push_back(entry);
    }));

    connections_.Add(parent->on_entry_removed.connect([this] (Entry::Ptr const& entry) {
      auto it = window_entries_.find(entry->parent_window());

      if (it != window_entries_.end())
      {
        auto& entries = it->second;
        entries.erase(std::remove(entries.begin(), entries.end(), entry), entries.end());

        if (entries.empty())
          window_entries_.erase(it);
      }
    }));
  }

  connection::Manager connections_;
  std::unordered_map<uint32_t, Indicator::Entries> window_entries_;
};

AppmenuIndicator::AppmenuIndicator(std::string const& name)
  : Indicator(name)
  , impl_(new AppmenuIndicator::Impl(this))
{}

AppmenuIndicator::~AppmenuIndicator()
{}

void AppmenuIndicator::Sync(Indicator::Entries const& entries)
{
  std::unordered_set<uint32_t> changed_windows;
  connection::Wrapper added_conn(on_entry_added.connect([this, &changed_windows] (Entry::Ptr const& entry) {
    changed_windows.insert(entry->parent_window());
  }));

  connection::Wrapper rm_conn(on_entry_removed.connect([this, &changed_windows] (Entry::Ptr const& entry) {
    changed_windows.insert(entry->parent_window());
  }));

  Indicator::Sync(entries);

  for (uint32_t win : changed_windows)
    updated_win.emit(win);
}

Indicator::Entries const& AppmenuIndicator::GetEntriesForWindow(uint32_t parent_window) const
{
  auto it = impl_->window_entries_.find(parent_window);

  if (it != impl_->window_entries_.end())
    return it->second;

  return empty_entries_;
}

void AppmenuIndicator::ShowAppmenu(unsigned int xid, int x, int y) const
{
  on_show_appmenu.emit(xid, x, y);
}

} // namespace indicator
} // namespace unity
