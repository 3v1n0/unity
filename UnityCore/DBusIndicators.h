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

#ifndef UNITY_DBUSINDICATORS_H
#define UNITY_DBUSINDICATORS_H

#include "Indicators.h"


namespace unity
{
namespace indicator
{

// Connects to the remote panel service (unity-panel-service) and translates
// that into something that the panel can show
class DBusIndicators : public Indicators
{
public:
  typedef std::shared_ptr<DBusIndicators> Ptr;

  DBusIndicators(bool lockscreen_mode = false);
  ~DBusIndicators();

  void ShowEntriesDropdown(Indicator::Entries const&, Entry::Ptr const&, unsigned xid, int x, int y);
  void SyncGeometries(std::string const& name, EntryLocationMap const& locations);

protected:
  virtual void OnEntryScroll(std::string const& entry_id, int delta);
  virtual void OnEntryShowMenu(std::string const& entry_id, unsigned int xid, int x, int y, unsigned int button);
  virtual void OnEntrySecondaryActivate(std::string const& entry_id);
  virtual void OnShowAppMenu(unsigned int xid, int x, int y);

  DBusIndicators(std::string const& dbus_name);
  bool IsConnected() const;

private:
  class Impl;
  std::unique_ptr<Impl> pimpl;
};

}
}

#endif
