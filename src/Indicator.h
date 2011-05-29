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

#ifndef UNITY_INDICATOR_H
#define UNITY_INDICATOR_H

#include <string>
#include <vector>

#include <gio/gio.h>
#include <dee.h>

#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

#include "IndicatorEntry.h"

namespace unity {
namespace indicator {

class Indicator : public sigc::trackable, boost::noncopyable
{
public:
  typedef boost::shared_ptr<Indicator> Ptr;
  typedef std::vector<Entry::Ptr> Entries;

  Indicator(std::string const& name);

  std::string const& name() const;

  void Sync(Entries const& new_entries);

  void OnEntryShowMenu(std::string const& entry_id, int x, int y, int timestamp, int button);
  void OnEntryScroll(std::string const& entry_id, int delta);

  // Signals
  sigc::signal<void, Entry::Ptr const&> on_entry_added;
  sigc::signal<void, std::string const&, int, int, int, int> on_show_menu;
  sigc::signal<void, std::string const&, int> on_scroll;

private:
  Entries entries_;
  std::string name_;
};

}
}

#endif // INDICATOR_OBJECT_PROXY_REMOTE_H
