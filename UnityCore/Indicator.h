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

#ifndef UNITY_INDICATOR_H
#define UNITY_INDICATOR_H

#include <vector>
#include <boost/utility.hpp>
#include <sigc++/connection.h>

#include "IndicatorEntry.h"
#include "ConnectionManager.h"


namespace unity
{
namespace indicator
{

class Indicator : public sigc::trackable, boost::noncopyable
{
public:
  typedef std::shared_ptr<Indicator> Ptr;
  typedef std::list<Entry::Ptr> Entries;

  Indicator(std::string const& name);
  virtual ~Indicator();

  std::string const& name() const;

  virtual bool IsAppmenu() const { return false; }

  void Sync(Entries const& new_entries);
  Entry::Ptr GetEntry(std::string const& entry_id) const;
  int EntryIndex(std::string const& entry_id) const;
  Entries const& GetEntries() const;

  // Signals
  sigc::signal<void> updated;
  sigc::signal<void, Entry::Ptr const&> on_entry_added;
  sigc::signal<void, std::string const&> on_entry_removed;
  sigc::signal<void, std::string const&, unsigned, int, int, unsigned> on_show_menu;
  sigc::signal<void, std::string const&> on_secondary_activate;
  sigc::signal<void, std::string const&, int> on_scroll;

protected:
  void OnEntryShowMenu(std::string const& entry_id, unsigned xid, int x, int y, unsigned button);
  void OnEntrySecondaryActivate(std::string const& entry_id);
  void OnEntryScroll(std::string const& entry_id, int delta);

  Entries entries_;
  std::string name_;
  std::map<Entry::Ptr, connection::Manager> entries_connections_;

  friend std::ostream& operator<<(std::ostream& out, Indicator const& i);
};


}
}

#endif // UNITY_INDICATOR_H
