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

#include "PlaceEntryRemote.h"

#include <glib/gi18n-lib.h>

#define DBUS_PATH "DBusObjectPath"

PlaceEntryRemote::PlaceEntryRemote (Place *parent)
: _parent (parent),
  _dbus_path (NULL),
  _name (NULL),
  _icon (NULL),
  _description (NULL),
  _position (0),
  _mimetypes (NULL),
  _sensitive (true),
  _active (false),
  _valid (false),
  _show_in_global (false)
{
}

PlaceEntryRemote::~PlaceEntryRemote ()
{
  g_free (_name);
  g_free (_dbus_path);
  g_free (_icon);
  g_free (_description);
  g_strfreev (_mimetypes);
}

void
PlaceEntryRemote::InitFromKeyFile (GKeyFile    *key_file,
                                   const gchar *group)
{
  GError *error = NULL;
  gchar  *domain;
  gchar  *name;
  gchar  *icon;
  gchar  *description;

  _dbus_path = g_key_file_get_string (key_file, group, DBUS_PATH, &error);
  if (_dbus_path == NULL
      || g_strcmp0 (_dbus_path, "") == 0
      || _dbus_path[0] != '/'
      || error)
  {
    g_warning ("Unable to load PlaceEntry '%s': Does not contain valid '"DBUS_PATH"' (%s)",
               group,
               error ? error->message : "");
    if (error)
      g_error_free (error);
    return;
  }

  domain = g_key_file_get_string (key_file, G_KEY_FILE_DESKTOP_GROUP,
                                  "X-Ubuntu-Gettext-Domain", &error);
  if (error)
  {
    // I'm messaging here because it probably should contain one
    g_message ("PlaceEntry %s does not contain a translation gettext name: %s",
               group,
               error->message);
    g_error_free (error);
    error = NULL;
  }

  name = g_key_file_get_string (key_file, group, G_KEY_FILE_DESKTOP_KEY_NAME, NULL);
  icon = g_key_file_get_string (key_file, group, G_KEY_FILE_DESKTOP_KEY_ICON, NULL);
  description = g_key_file_get_string (key_file, group, "Description", NULL);

  if (domain)
  {
    _name = dgettext (domain, name);
    _icon = dgettext (domain, icon);
    _description = dgettext (domain, description);
  }
  else
  {
    _name = name;
    _icon = icon;
    _description = description;

    name = NULL;
    icon = NULL;
    description = NULL;
  }

  /* Finally the two that should default to true */
  if (g_key_file_has_key (key_file, group, "ShowGlobal", NULL))
    _show_in_global = g_key_file_get_boolean (key_file, group, "ShowGlobal", NULL);

  if (g_key_file_has_key (key_file, group, "ShowEntry", NULL))
    _sensitive = g_key_file_get_boolean (key_file, group, "ShowEntry", NULL);

  _valid = true;

  g_debug ("PlaceEntry: %s", _name);

  g_free (name);
  g_free (icon);
  g_free (description);
}

/* Other methods */
bool
PlaceEntryRemote::IsValid ()
{
  return _valid;
}

const gchar *
PlaceEntryRemote::GetPath ()
{
  return _dbus_path;
}

void
PlaceEntryRemote::Update (const gchar  *dbus_path,
                          const gchar  *name,
                          const gchar  *icon,
                          guint32       position,
                          const gchar **mimetypes,
                          gboolean      sensitive,
                          const gchar  *sections_model_name,
                          GVariantIter *hints,
                          const gchar  *entry_renderer,
                          const gchar  *entry_groups_model,
                          const gchar  *entry_results_model,
                          GVariantIter *entry_hints,
                          const gchar  *global_renderer,
                          const gchar  *global_groups_model,
                          const gchar  *global_results_model,
                          GVariantIter *global_hints)
{

}


/* Overrides */
const gchar *
PlaceEntryRemote::GetName ()
{
  return _name;
}

const gchar *
PlaceEntryRemote::GetIcon ()
{
  return _icon;
}

const gchar *
PlaceEntryRemote::GetDescription ()
{
  return _description;
}

guint32
PlaceEntryRemote::GetPosition  ()
{
  return _position;
}

const gchar **
PlaceEntryRemote::GetMimetypes ()
{
  return (const gchar **)_mimetypes;
}

const std::map<gchar *, gchar *>&
PlaceEntryRemote::GetHints ()
{
  return _hints;
}

bool
PlaceEntryRemote::IsSensitive ()
{
  return _sensitive;
}

bool
PlaceEntryRemote::IsActive ()
{
  return _active;
}

bool
PlaceEntryRemote::ShowInGlobal ()
{
  return _show_in_global;
}

Place *
PlaceEntryRemote::GetParent ()
{
  return _parent;
}
