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

#include "GLibDBusNameWatcher.h"
#include "GLibWrapper.h"

namespace unity
{
namespace glib
{

DBusNameWatcher::DBusNameWatcher(std::string const& name, GBusType bus_type, GBusNameWatcherFlags flags)
  : watcher_id_(g_bus_watch_name(bus_type, name.c_str(), flags,
      [] (GDBusConnection *connection, const gchar *name, const gchar *name_owner, gpointer self) {
        static_cast<DBusNameWatcher*>(self)->appeared.emit(gchar_to_string(name), gchar_to_string(name_owner));
      },
      [] (GDBusConnection *connection, const gchar *name, gpointer self) {
        static_cast<DBusNameWatcher*>(self)->vanished.emit(gchar_to_string(name));
      }, this, nullptr))
{}

DBusNameWatcher::~DBusNameWatcher()
{
  g_bus_unwatch_name(watcher_id_);
}

} // namespace glib
} // namespace unity
