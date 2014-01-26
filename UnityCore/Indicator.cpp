// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010-2011 Canonical Ltd
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
 *              Marco Trevisan (Treviño) <mail@3v1n0.net>
 */

#include "Indicator.h"

#include <iostream>
#include <algorithm>


namespace unity
{
namespace indicator
{

Indicator::Indicator(std::string const& name)
  : name_(name)
{}

Indicator::~Indicator()
{
  for (auto const& entry : entries_)
    on_entry_removed.emit(entry->id());
}

std::string const& Indicator::name() const
{
  return name_;
}

Indicator::Entries const& Indicator::GetEntries() const
{
  return entries_;
}

void Indicator::Sync(Indicator::Entries const& new_entries)
{
  bool added = false;
  Entries to_rm;

  if (!entries_.empty())
  {
    for (auto const& entry : entries_)
    {
      if (std::find(new_entries.begin(), new_entries.end(), entry) == new_entries.end())
        to_rm.push_back(entry);
    }
  }

  for (auto const& entry : to_rm)
  {
    entries_connections_.erase(entry);
    on_entry_removed.emit(entry->id());
    entries_.remove(entry);
  }

  for (auto const& new_entry : new_entries)
  {
    if (GetEntry(new_entry->id()))
      continue;

    // Just add the new entry, and connect it up.
    connection::Manager& new_entry_connections = entries_connections_[new_entry];

    new_entry_connections.Add(new_entry->on_show_menu.connect([this] (std::string const& entry_id, unsigned xid, int x, int y, unsigned button) {
      on_show_menu.emit(entry_id, xid, x, y, button);
    }));

    new_entry_connections.Add(new_entry->on_show_dropdown_menu.connect([this] (std::string const& entry_id, unsigned xid, int x, int y) {
      on_show_dropdown_menu.emit(entry_id, xid, x, y);
    }));

    new_entry_connections.Add(new_entry->on_secondary_activate.connect([this] (std::string const& entry_id) {
      on_secondary_activate.emit(entry_id);
    }));

    new_entry_connections.Add(new_entry->on_scroll.connect([this] (std::string const& entry_id, int delta) {
      on_scroll.emit(entry_id, delta);
    }));

    entries_.push_back(new_entry);
    on_entry_added.emit(new_entry);

    added = true;
  }

  if (!to_rm.empty() || added)
    updated.emit();
}

Entry::Ptr Indicator::GetEntry(std::string const& entry_id) const
{
  for (auto const& entry : entries_)
    if (entry->id() == entry_id)
      return entry;

  return Entry::Ptr();
}

int Indicator::EntryIndex(std::string const& entry_id) const
{
  int i = 0;
  for (auto const& entry : entries_)
  {
    if (entry->id() == entry_id)
    {
      return i;
    }
    ++i;
  }

  return -1;
}

std::ostream& operator<<(std::ostream& out, Indicator const& i)
{
  out << "<Indicator " << i.name() << std::endl;
  for (auto const& entry : i.entries_)
  {
    out << "\t" << entry << std::endl;
  }
  out << "\t>" << std::endl;
  return out;
}


} // namespace indicator
} // namespace unity
