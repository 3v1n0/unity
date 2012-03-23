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


namespace unity
{
namespace indicator
{

Indicator::Indicator(std::string const& name)
  : name_(name)
{
}

Indicator::~Indicator()
{
  for (auto entry : entries_)
    on_entry_removed.emit(entry->id());

  entries_.clear();

  for (auto it : entries_connections_)
  {
    for (auto conn : it.second)
      conn.disconnect();

    it.second.clear();
  }
}

std::string const& Indicator::name() const
{
  return name_;
}

Indicator::Entries Indicator::GetEntries() const
{
  return entries_;
}

void Indicator::Sync(Indicator::Entries const& new_entries)
{
  Entries to_rm;

  if (entries_.size() == 0)
  {
    to_rm = entries_;
  }
  else
  {
    for (auto entry : entries_)
    {
      if (std::find(new_entries.begin(), new_entries.end(), entry) == new_entries.end())
      {
        to_rm.push_back(entry);
      }
    }
  }

  for (auto entry : to_rm) {
    for (auto conn : entries_connections_[entry])
      conn.disconnect();

    entries_connections_[entry].clear();

    on_entry_removed.emit(entry->id());
    entries_.remove(entry);
  }

  for (auto new_entry : new_entries)
  {
    if (GetEntry(new_entry->id()))
      continue;

    // Just add the new entry, and connect it up.
    sigc::connection conn;
    std::vector<sigc::connection>& new_entry_connections = entries_connections_[new_entry];

    conn = new_entry->on_show_menu.connect(sigc::mem_fun(this, &Indicator::OnEntryShowMenu));
    new_entry_connections.push_back(conn);

    conn = new_entry->on_secondary_activate.connect(sigc::mem_fun(this, &Indicator::OnEntrySecondaryActivate));
    new_entry_connections.push_back(conn);

    conn = new_entry->on_scroll.connect(sigc::mem_fun(this, &Indicator::OnEntryScroll));
    entries_.push_back(new_entry);
    new_entry_connections.push_back(conn);

    on_entry_added.emit(new_entry);
  }
}

Entry::Ptr Indicator::GetEntry(std::string const& entry_id) const
{
  for (auto entry : entries_)
    if (entry->id() == entry_id)
      return entry;

  return Entry::Ptr();
}

void Indicator::OnEntryShowMenu(std::string const& entry_id, unsigned int xid,
                                int x, int y, unsigned int button, unsigned int timestamp)
{
  on_show_menu.emit(entry_id, xid, x, y, button, timestamp);
}

void Indicator::OnEntrySecondaryActivate(std::string const& entry_id,
                                         unsigned int timestamp)
{
  on_secondary_activate.emit(entry_id, timestamp);
}

void Indicator::OnEntryScroll(std::string const& entry_id, int delta)
{
  on_scroll.emit(entry_id, delta);
}

std::ostream& operator<<(std::ostream& out, Indicator const& i)
{
  out << "<Indicator " << i.name() << std::endl;
  for (auto entry : i.entries_)
  {
    out << "\t" << entry << std::endl;
  }
  out << "\t>" << std::endl;
  return out;
}


} // namespace indicator
} // namespace unity
