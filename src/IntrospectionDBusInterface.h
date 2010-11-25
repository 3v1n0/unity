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

#ifndef _STATE_INTROSPECTION_DBUS_INTERFACE_H
#define _STATE_INTROSPECTION_DBUS_INTERFACE_H 1

#include <glib.h>
#include <gio/gio.h>
#include "Introspectable.h"

class IntrospectionDBusInterface
{
public:
  IntrospectionDBusInterface  (Introspectable *introspectable);
  ~IntrospectionDBusInterface ();

  /*static void DBusMethodCall (GDBusConnection *connection, const gchar *sender,
                              const gchar *objectPath, const gchar *ifaceName,
                              const gchar *methodName, GVariant *parameters,
                              GDBusMethodInvocation *invocation, gpointer data);	*/

private:
  /* methods */

  static void OnBusAcquired (GDBusConnection *connection, const gchar *name, gpointer data);

  static void OnNameAcquired (GDBusConnection *connection, const gchar *name, gpointer data);

  static void OnNameLost (GDBusConnection *connection, const gchar *name, gpointer data);

  //static GVariant *GetState (const char *piece);

  static GVariant *BuildFakeReturn ();

  /* members */
  guint						_owner_id;
};

#endif
