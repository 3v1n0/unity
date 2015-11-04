// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2015 Canonical Ltd
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
 * Authored by: Marco Trevisan (Treviño) <marco.trevisan@canonical.com>
 */

#ifndef UNITY_GLIB_DBUS_NAME_WATCHER_H
#define UNITY_GLIB_DBUS_NAME_WATCHER_H

#include <gio/gio.h>
#include <memory>
#include <sigc++/signal.h>

namespace unity
{
namespace glib
{

class DBusNameWatcher
{
public:
  typedef std::shared_ptr<DBusNameWatcher> Ptr;

  DBusNameWatcher(std::string const& name, GBusType bus_type = G_BUS_TYPE_SESSION, GBusNameWatcherFlags flags = G_BUS_NAME_WATCHER_FLAGS_NONE);
  virtual ~DBusNameWatcher();

  sigc::signal<void, std::string const& /* name*/, std::string const& /* name owner*/> appeared;
  sigc::signal<void, std::string const& /* name */> vanished;

private:
  // not copyable class
  DBusNameWatcher(DBusNameWatcher const&) = delete;
  DBusNameWatcher& operator=(DBusNameWatcher const&) = delete;

  uint32_t watcher_id_;
};

} // namespace glib
} // namespace unity

#endif //UNITY_GLIB_DBUS_NAME_WATCHER_H
