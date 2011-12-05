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
 * Authored by: Alex Launi <alex.launi@canonical.com>
 */

#include <gio/gio.h>

#ifndef _DEBUG_DBUS_INTERFACE_H
#define _DEBUG_DBUS_INTERFACE_H 1

#define DBUS_BUS_NAME                 "com.canonical.Unity.Debug"
#define DBUS_DEBUG_OBJECT_PATH        "/com/canonical/Unity/Debug"
#define DBUS_INTROSPECTION_IFACE_NAME "com.canonical.Unity.Debug.Introspection"

class CompScreen;

namespace unity
{
namespace debug
{
class Introspectable;

class DebugDBusInterface
{
public:
  DebugDBusInterface(Introspectable* introspectable, CompScreen* uscreen);
  ~DebugDBusInterface();

private:
  /* methods */
  static void OnBusAcquired(GDBusConnection* connection, const gchar* name, gpointer data);

  static void OnNameAcquired(GDBusConnection* connection, const gchar* name, gpointer data);

  static void OnNameLost(GDBusConnection* connection, const gchar* name, gpointer data);

  static GVariant* BuildFakeReturn();

  /* members */
  guint           _owner_id;
};
}
}

#endif /* _DEBUG_DBUS_INTERFACE_H */
