// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011 Canonical Ltd
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
 * Authored by: Mikkel Kamstrup Erlandsen <mikkel.kamstrup@canonical.com>
 */

#include "LauncherEntryRemoteModel.h"

static void on_launcher_entry_signal_received (GDBusConnection *connection,
                                               const gchar *sender_name,
                                               const gchar *object_path,
                                               const gchar *interface_name,
                                               const gchar *signal_name,
                                               GVariant *parameters,
                                               gpointer user_data);

/**
 * Helper class implementing the remote API to control the icons in the
 * launcher. Also known as the com.canonical.Unity.LauncherEntry DBus API.
 * It enables clients to dynamically control their launcher icons by
 * adding a counter, progress indicator, or emblem to it. It also supports
 * adding a quicklist menu by means of the dbusmenu library.
 *
 * We take care to print out any client side errors or oddities in detail,
 * in order to help third party developers as much as possible when integrating
 * with Unity.
 */
LauncherEntryRemoteModel::LauncherEntryRemoteModel()
{
  GError          *error;

  launcher_entry_dbus_signal_id = 0;

  error = NULL;
  conn = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
  if (error)
    {
      g_warning ("Unable to connect to session bus: %s", error->message);
      g_error_free (error);
      return;
    }
  
  /* Listen for *all* signals on the "com.canonical.Unity.LauncherEntry"
   * interface, no matter who the sender is */
  launcher_entry_dbus_signal_id =
      g_dbus_connection_signal_subscribe (conn,
                                          NULL,                       // sender
                                          "com.canonical.Unity.LauncherEntry",
                                          NULL,                       // member
                                          NULL,                       // path
                                          NULL,                       // arg0
                                          G_DBUS_SIGNAL_FLAGS_NONE,
                                          on_launcher_entry_signal_received,
                                          this,
                                          NULL);
}

LauncherEntryRemoteModel::~LauncherEntryRemoteModel()
{
  if (launcher_entry_dbus_signal_id && conn)
    g_dbus_connection_signal_unsubscribe (conn, launcher_entry_dbus_signal_id);
  
  if (conn)
    g_object_unref (conn);
}

int
LauncherEntryRemoteModel::Size ()
{
  return _entries.size ();
}

std::list<LauncherEntryRemote*>::iterator
LauncherEntryRemoteModel::begin ()
{
  return _entries.begin ();
}

std::list<LauncherEntryRemote*>::iterator
LauncherEntryRemoteModel::end ()
{
  return _entries.end ();
}

std::list<LauncherEntryRemote*>::reverse_iterator
LauncherEntryRemoteModel::rbegin ()
{
  return _entries.rbegin ();
}

std::list<LauncherEntryRemote*>::reverse_iterator
LauncherEntryRemoteModel::rend ()
{
  return _entries.rend ();
}

/* Called when the signal com.canonical.Unity.LauncherEntry.Update is received.
 * The app_uri looks like "application://firefox.desktop". The properties
 * hashtable is a map from strings to GVariants with the property value.
 */
void
LauncherEntryRemoteModel::OnUpdateReceived (const gchar *app_uri,
                                            GHashTable  *props)
{
  g_return_if_fail (app_uri != NULL);
  g_return_if_fail (props != NULL);


}

static void
on_launcher_entry_signal_received (GDBusConnection *connection,
                                   const gchar     *sender_name,
                                   const gchar     *object_path,
                                   const gchar     *interface_name,
                                   const gchar     *signal_name,
                                   GVariant        *parameters,
                                   gpointer         user_data)
{
  LauncherEntryRemoteModel *self;

  self = static_cast<LauncherEntryRemoteModel *> (user_data);

  if (parameters == NULL)
    {
      g_warning ("Received DBus signal '%s.%s' with empty payload from %s",
                 interface_name, signal_name, sender_name);
      return;
    }

  if (g_strcmp0 (signal_name, "Update") == 0)
    {
      if (!g_variant_is_of_type (parameters, G_VARIANT_TYPE ("(sa{sv})")))
        {
          g_warning ("Received 'com.canonical.Unity.LauncherEntry.Update' with"
                     " illegal payload signature '%s'. Expected '(sa{sv})'.",
                     g_variant_get_type_string (parameters));
          return;
        }
      // FIXME parse props
      self->OnUpdateReceived (NULL, NULL);
    }
  else
    {
      g_warning ("Unknown signal '%s.%s' from %s",
                 interface_name, signal_name, sender_name);
    }


}
