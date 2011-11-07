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

#ifndef _DEBUG_DBUS_INTERFACE_H
#define _DEBUG_DBUS_INTERFACE_H 1

class CompScreen;

namespace unity
{
class Introspectable;

class DebugDBusInterface
{
public:
  DebugDBusInterface(Introspectable* introspectable, CompScreen* uscreen);
  ~DebugDBusInterface();

  static constexpr gchar* UNITY_DBUS_BUS_NAME = (gchar*) "com.canonical.Unity";
  static constexpr gchar* UNITY_DBUS_DEBUG_OBJECT_PATH = (gchar*) "/com/canonical/Unity/Debug";
  static constexpr gchar* UNITY_DBUS_AP_IFACE_NAME = (gchar*) "com.canonical.Unity.Debug.Autopilot";
  static constexpr gchar* UNITY_DBUS_INTROSPECTION_IFACE_NAME = (gchar*) "com.canonical.Unity.Debug.Introspection";
  static constexpr gchar* UNITY_DBUS_AP_SIG_TESTFINISHED = (gchar*)  "TestFinished";
  static constexpr gchar* SI_METHOD_NAME_GETSTATE = (gchar*) "GetState";
  static constexpr gchar* AP_METHOD_NAME_STARTTEST = (gchar*) "StartTest";

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

#endif /* _DEBUG_DBUS_INTERFACE_H */
