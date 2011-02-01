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

static void nux_object_destroy_notify (nux::Object *obj);

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

  _launcher_entry_dbus_signal_id = 0;

  error = NULL;
  _conn = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
  if (error)
    {
      g_warning ("Unable to connect to session bus: %s", error->message);
      g_error_free (error);
      return;
    }
  
  _entries_by_uri = g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
                                           (GDestroyNotify) nux_object_destroy_notify);

  /* Listen for *all* signals on the "com.canonical.Unity.LauncherEntry"
   * interface, no matter who the sender is */
  _launcher_entry_dbus_signal_id =
      g_dbus_connection_signal_subscribe (_conn,
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
  g_hash_table_destroy (_entries_by_uri);
  _entries_by_uri = NULL;

  if (_launcher_entry_dbus_signal_id && _conn)
    {
      g_dbus_connection_signal_unsubscribe (_conn,
                                            _launcher_entry_dbus_signal_id);
      _launcher_entry_dbus_signal_id = 0;
    }
  
  if (_conn)
    {
      g_object_unref (_conn);
      _conn = NULL;
    }
}

/**
 * Get a pointer to the default LauncherEntryRemoteModel singleton for this
 * process. The return value should not be freed.
 */
LauncherEntryRemoteModel*
LauncherEntryRemoteModel::GetDefault()
{
  static LauncherEntryRemoteModel *singleton = NULL;

  if (singleton == NULL)
    singleton = new LauncherEntryRemoteModel ();

  return singleton;
}

/**
 * Return the number of unique LauncherEntryRemote objects managed by the model.
 * The entries are identified by their LauncherEntryRemote::AppUri property.
 */
guint
LauncherEntryRemoteModel::Size ()
{
  return g_hash_table_size (_entries_by_uri);
}

/**
 * Return a pointer to a LauncherEntryRemote if there is one for app_uri,
 * otherwise NULL. The returned object should not be freed.
 */
LauncherEntryRemote*
LauncherEntryRemoteModel::LookupByUri (const gchar *app_uri)
{
  gpointer entry;

  g_return_val_if_fail (app_uri != NULL, NULL);

  entry = g_hash_table_lookup (_entries_by_uri,  app_uri);
  return static_cast<LauncherEntryRemote *> (entry);
}

/**
 * Get a list of all application URIs which have registered with the launcher
 * API. The returned GList should be freed with g_list_free(), but the URIs
 * should not be changed or freed.
 */
GList*
LauncherEntryRemoteModel::GetUris ()
{
  return (GList*) g_hash_table_get_keys (_entries_by_uri);
}

/**
 * Add or update a remote launcher entry.
 *
 * If 'entry' has a floating reference it will be consumed.
 */
void
LauncherEntryRemoteModel::AddEntry (LauncherEntryRemote *entry)
{
  LauncherEntryRemote *existing_entry;

  g_return_if_fail (entry != NULL);

  entry->SinkReference ();

  existing_entry = LookupByUri (entry->AppUri ());
  if (existing_entry != NULL)
    {
      existing_entry->Update (entry);
      entry->UnReference ();
    }
  else
    {
      /* The ref on entry will be removed by the hash table itself */
      g_hash_table_insert (_entries_by_uri, g_strdup (entry->AppUri ()), entry);
      entry_added.emit (entry);
    }
}

/**
 * Add or update a remote launcher entry.
 *
 * If 'entry' has a floating reference it will be consumed.
 */
void
LauncherEntryRemoteModel::RemoveEntry (LauncherEntryRemote *entry)
{
  g_return_if_fail (entry != NULL);

  /* We need a temp ref on entry to keep it alive during signal emission */
  entry->SinkReference ();

  if (g_hash_table_remove (_entries_by_uri, entry->AppUri ()))
    {
      entry_removed.emit (entry);
    }

  entry->UnReference ();
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
  LauncherEntryRemote      *entry;

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

      if (sender_name == NULL)
        {
          g_critical ("Received 'com.canonical.Unity.LauncherEntry.Update' from"
                      " an undefined sender. This may happen if you are trying "
                      "to run Unity on a p2p DBus connection.");
          return;
        }

      entry = new LauncherEntryRemote (sender_name, parameters);
      self->AddEntry (entry); // consumes floating ref on entry
    }
  else
    {
      g_warning ("Unknown signal '%s.%s' from %s",
                 interface_name, signal_name, sender_name);
    }


}

static void
nux_object_destroy_notify (nux::Object *obj)
{
  if (G_LIKELY (obj != NULL))
    obj->UnReference();
}
