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
  typedef boost::shared_ptr<DBusIndicators> Ptr;

  DBusIndicators();
  ~DBusIndicators();

  void SyncGeometries(std::string const& name,
                      EntryLocationMap const& locations);

  virtual void OnEntryScroll(std::string const& entry_id, int delta);
  virtual void OnEntryShowMenu(std::string const& entry_id,
                               int x, int y, int timestamp, int button);
  virtual void OnEntrySecondaryActivate(std::string const& entry_id,
                                        unsigned int timestamp);

  std::string name() const;
  std::string owner_name() const;
  bool using_local_service() const;

  // Due to the callback nature, the Impl class must be declared public, but
  // it is not available to anyone except the internal implementation.
  class Impl;
private:
  Impl* pimpl;
};

}
}

#endif
