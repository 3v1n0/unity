// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010 Canonical Ltd
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

#include "Indicator.h"

#include <iostream>


namespace unity {
namespace indicator {

Indicator::Indicator(std::string const& name)
  : name_(name)
{
}

std::string const& Indicator::name() const
{
  return name_;
}

void Indicator::Sync(Indicator::Entries const& new_entries)
{
  std::size_t curr_index = 0;

  for (std::size_t new_index = 0, size = new_entries.size(); new_index < size;
       ++new_index, ++curr_index)
  {
    if (curr_index < entries_.size())
    {
      Entry& our_entry = *(entries_[curr_index]);
      Entry const& new_entry = *(new_entries[new_index]);
      our_entry = new_entry;
    }
    else
    {
      // Just add the new entry, and connect it up.
      Entry::Ptr new_entry = new_entries[new_index];
      entries_.push_back(new_entry);
      new_entry->on_show_menu.connect(sigc::mem_fun(this, &Indicator::OnEntryShowMenu));
      new_entry->on_scroll.connect(sigc::mem_fun(this, &Indicator::OnEntryScroll));
      on_entry_added.emit(new_entry);
    }
  }

  // Mark any other entries as not used.
  for (std::size_t size = entries_.size(); curr_index < size; ++curr_index)
  {
    entries_[curr_index]->MarkUnused();
  }
}

Entry::Ptr Indicator::GetEntry(std::string const& entry_id) const
{
  for (Entries::const_iterator i = entries_.begin(),
         end = entries_.end(); i != end; ++i)
  {
    if ((*i)->id() == entry_id)
      return *i;
  }
  return Entry::Ptr();
}

void Indicator::OnEntryShowMenu(std::string const& entry_id,
                                int x, int y,
                                int timestamp, int button)
{
  on_show_menu.emit(entry_id, x, y, timestamp, button);
}

void Indicator::OnEntryScroll(std::string const& entry_id, int delta)
{
  on_scroll.emit(entry_id, delta);
}

std::ostream& operator<<(std::ostream& out, Indicator const& i)
{
  out << "<Indicator " << i.name() << std::endl;
  for (Indicator::Entries::const_iterator iter = i.entries_.begin(),
         end = i.entries_.end(); iter != end; ++iter)
  {
    out << "\t" << **iter << std::endl;
  }
  out << "\t>" << std::endl;
  return out;
}


} // namespace indicator
} // namespace unity
