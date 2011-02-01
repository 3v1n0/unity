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

#include "LauncherEntryRemote.h"

/**
 * Create a new LauncherEntryRemote parsed from the raw DBus wire format
 * of the com.canonical.Unity.LauncherEntry.Update signal '(sa{sv})'. The
 * dbus_name argument should be the unique bus name of the process owning
 * the launcher entry.
 *
 * You don't normally need to use this constructor yourself. The
 * LauncherEntryRemoteModel will do that for you when needed.
 */
LauncherEntryRemote::LauncherEntryRemote(const gchar *dbus_name, GVariant *val)
{
  gchar        *app_uri, *prop_key;
  GVariantIter *prop_iter;
  GVariant     *prop_value;

  g_return_if_fail (dbus_name != NULL);
  g_return_if_fail (val != NULL);

  _dbus_name = g_strdup (dbus_name);

  _emblem = NULL;
  _count = G_GINT64_CONSTANT (0);
  _progress = 0.0;
  _quicklist = NULL;
  
  _emblem_visible = FALSE;
  _count_visible = FALSE;
  _progress_visible = FALSE;

  g_variant_ref_sink (val);
  g_variant_get (val, "(sa{sv})", &app_uri, &prop_iter);

  _app_uri = app_uri; // steal ref

  while (g_variant_iter_loop (prop_iter, "{sv}", &prop_key, &prop_value))
    {
      if (g_str_equal ("emblem", prop_key))
        g_variant_get (prop_value, "s", &_emblem);
      else if (g_str_equal ("count", prop_key))
        _count = g_variant_get_int64 (prop_value);
      else if (g_str_equal ("progress", prop_key))
        _progress = g_variant_get_double (prop_value);
      else if (g_str_equal ("emblem-visible", prop_key))
        _emblem_visible = g_variant_get_boolean (prop_value);
      else if (g_str_equal ("count-visible", prop_key))
        _count_visible = g_variant_get_boolean (prop_value);
      else if (g_str_equal ("progress-visible", prop_key))
        _progress_visible = g_variant_get_boolean (prop_value);
      else if (g_str_equal ("quicklist", prop_key))
        {
          const gchar *ql_path;
          ql_path = g_variant_get_string (prop_value,  NULL);
          _quicklist = dbusmenu_client_new (_dbus_name, ql_path);
        }
    }

  g_variant_iter_free (prop_iter);
  g_variant_unref (val);
}

LauncherEntryRemote::~LauncherEntryRemote()
{
  if (_dbus_name)
    {
      g_free (_dbus_name);
      _dbus_name = NULL;
    }

  if (_app_uri)
    {
      g_free (_app_uri);
      _app_uri = NULL;
    }
  
  if (_emblem)
    {
      g_free (_emblem);
      _emblem = NULL;
    }

  if (_quicklist)
    {
      g_object_unref (_quicklist);
      _quicklist = NULL;
    }
}

/**
 * The appuri property is on the form application://$desktop_file_id and is
 * used within the LauncherEntryRemoteModel to uniquely identify the various
 * applications.
 *
 * Note that a desktop file id is defined to be the base name of the desktop
 * file including the extension. Eg. gedit.desktop. Thus a full appuri could be
 * application://gedit.desktop.
 */
const gchar*
LauncherEntryRemote::AppUri()
{
  return _app_uri;
}

/**
 * Return the unique DBus name for the remote party owning this launcher entry.
 */
const gchar*
LauncherEntryRemote::DBusName()
{
  return _dbus_name;
}

const gchar*
LauncherEntryRemote::Emblem()
{
  return _emblem;
}

gint64
LauncherEntryRemote::Count()
{
  return _count;
}

gdouble
LauncherEntryRemote::Progress()
{
  return _progress;
}

/**
 * Return a pointer to the current quicklist of the launcher entry; NULL if
 * it's unset.
 *
 * The returned object should not be freed.
 */
DbusmenuClient*
LauncherEntryRemote::Quicklist()
{
  return _quicklist;
}

gboolean
LauncherEntryRemote::EmblemVisible()
{
  return _emblem_visible;
}

gboolean
LauncherEntryRemote::CountVisible()
{
  return _count_visible;
}

gboolean
LauncherEntryRemote::ProgressVisible()
{
  return _progress_visible;
}

void
LauncherEntryRemote::SetEmblem(const gchar* emblem)
{
  if (g_strcmp0 (_emblem, emblem) == 0)
    return;

  if (_emblem)
    g_free (_emblem);

  _emblem = g_strdup (emblem);
  emblem_changed.emit ();
}

void
LauncherEntryRemote::SetCount(gint64 count)
{
  if (_count == count)
    return;

  _count = count;
  count_changed.emit ();
}

void
LauncherEntryRemote::SetProgress(gdouble progress)
{
  if (_progress == progress)
    return;

  _progress = progress;
  progress_changed.emit ();
}

/**
 * Set the quicklist of this entry to be the Dbusmenu on the given path.
 * If entry already has a quicklist with the same path this call is a no-op.
 *
 * To unset the quicklist pass in a NULL path.
 */
void
LauncherEntryRemote::SetQuicklistPath(const gchar *dbus_path)
{
  /* Check if existing quicklist have exact same path
   * and ignore the change in that case */
  if (_quicklist)
    {
      gchar *ql_path;
      g_object_get (_quicklist, DBUSMENU_CLIENT_PROP_DBUS_OBJECT, &ql_path, NULL);
      if (g_strcmp0 (dbus_path, ql_path) == 0)
        {
          g_free (ql_path);
          return;
        }

      /* Prepare for a new quicklist */
      g_free (ql_path);
      g_object_unref (_quicklist);
    }
  else if (_quicklist == NULL && dbus_path == NULL)
    return;

  if (dbus_path != NULL)
    _quicklist = dbusmenu_client_new (_dbus_name, dbus_path);
  else
    _quicklist = NULL;

  quicklist_changed.emit ();
}

/**
 * Set the quicklist of this entry to a given DbusmenuClient.
 * If entry already has a quicklist with the same path this call is a no-op.
 *
 * To unset the quicklist for this entry pass in NULL as the quicklist to set.
 */
void
LauncherEntryRemote::SetQuicklist(DbusmenuClient *quicklist)
{
  /* Check if existing quicklist have exact same path as the new one
   * and ignore the change in that case */
  if (_quicklist)
    {
      gchar *ql_path, *new_ql_path;
      g_object_get (_quicklist, DBUSMENU_CLIENT_PROP_DBUS_OBJECT, &ql_path, NULL);

      if (quicklist == NULL)
        new_ql_path = NULL;
      else
        g_object_get (quicklist, DBUSMENU_CLIENT_PROP_DBUS_OBJECT, &new_ql_path, NULL);

      if (g_strcmp0 (new_ql_path, ql_path) == 0)
        {
          g_free (ql_path);
          g_free (new_ql_path);
          return;
        }

      /* Prepare for a new quicklist */
      g_free (ql_path);
      g_free (new_ql_path);
      g_object_unref (_quicklist);
    }
  else if (_quicklist == NULL && quicklist == NULL)
    return;

  if (quicklist == NULL)
    _quicklist = NULL;
  else
    _quicklist = (DbusmenuClient *) g_object_ref (quicklist);

  quicklist_changed.emit ();
}

void
LauncherEntryRemote::SetEmblemVisible(gboolean visible)
{
  if (_emblem_visible == visible)
    return;

  _emblem_visible = visible;
  emblem_visible_changed.emit ();
}

void
LauncherEntryRemote::SetCountVisible(gboolean visible)
{
  if (_count_visible == visible)
      return;

  _count_visible = visible;
  count_visible_changed.emit ();
}

void
LauncherEntryRemote::SetProgressVisible(gboolean visible)
{
  if (_progress_visible == visible)
      return;

  _progress_visible = visible;
  progress_visible_changed.emit ();
}

/**
 * Set all properties from 'other' on 'this'.
 */
void
LauncherEntryRemote::Update(LauncherEntryRemote *other)
{
  if (g_strcmp0 (DBusName(), other->DBusName()) != 0)
    {
      g_critical ("You can only call LauncherEntryRemote::Update() on entries"
                  " with the same unique DBus name. Saw %s and %s.",
                  DBusName(), other->DBusName());
      return;
    }

  SetEmblem (other->Emblem ());
  SetCount (other->Count ());
  SetProgress (other->Progress ());
  SetQuicklist (other->Quicklist());

  SetEmblemVisible (other->EmblemVisible ());
  SetCountVisible(other->CountVisible ());
  SetProgressVisible(other->ProgressVisible());
}
