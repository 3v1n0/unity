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

#include "PlaceRemote.h"

#include "PlaceEntryRemote.h"

#define PLACE_GROUP      "Place"
#define DBUS_NAME        "DBusName"
#define DBUS_PATH        "DBusObjectPath"

#define ENTRY_PREFIX     "Entry:"

#define ACTIVATION_GROUP "Activation"
#define URI_PATTERN      "URIPattern"
#define MIME_PATTERN     "MimetypePattern"

PlaceRemote::PlaceRemote (const char *path)
: _path (NULL),
  _dbus_name (NULL),
  _dbus_path (NULL),
  _uri_regex (NULL),
  _mime_regex (NULL)
{
  GKeyFile *key_file;
  GError   *error = NULL;

  g_debug ("Loading Place: %s", path);
  _path = g_strdup (path);

  // A .place file is a keyfile, so we create on representing the .place file to
  // read it's contents
  key_file = g_key_file_new ();
  if (!g_key_file_load_from_file (key_file,
                                  path,
                                  G_KEY_FILE_NONE,
                                  &error))
  {
    g_warning ("Unable to load Place %s: %s",
               path,
               error ? error->message : "Unknown");
    if (error)
      g_error_free (error);

    return;
  }

  if (!g_key_file_has_group (key_file, PLACE_GROUP))
  {
    g_warning ("Unable to load Place %s: keyfile does not contain a 'Place' group",
               path);
    g_key_file_free (key_file);
    return;
  }

  // Seems valid, so let's try and load the main values
  _dbus_name = g_key_file_get_string (key_file, PLACE_GROUP, DBUS_NAME, &error);
  if (error)
  {
    g_warning ("Unable to load Place %s: keyfile does not contain a '" DBUS_NAME "' key, %s",
               path,
               error->message);
    g_error_free (error);
    g_key_file_free (key_file);
    return;
  }

  _dbus_path = g_key_file_get_string (key_file, PLACE_GROUP, DBUS_PATH, &error);
  if (error)
  {
    g_warning ("Unable to load Place %s: keyfile does not contain a '" DBUS_PATH "' key, %s",
               path,
               error->message);
    g_error_free (error);
    g_key_file_free (key_file);
    return;
  }

  // Now the optional bits
  if (g_key_file_has_group (key_file, ACTIVATION_GROUP))
  {
    gchar *uri_match;
    gchar *mime_match;

    uri_match = g_key_file_get_string (key_file, ACTIVATION_GROUP, URI_PATTERN, &error);
    if (error)
    {
      // Fail silently as it's not a requirement
      g_error_free (error);
      error = NULL;
    }
    else if (uri_match)
    {
      _uri_regex = g_regex_new (uri_match,
                                (GRegexCompileFlags)0,
                                (GRegexMatchFlags)0,
                                &error);
      if (error)
      {
        g_warning ("Unable to compile '"URI_PATTERN"' regex for %s: %s",
                   path,
                   error->message);
        g_error_free (error);
        error = NULL;
      }
    }

    mime_match = g_key_file_get_string (key_file, ACTIVATION_GROUP, MIME_PATTERN, &error);
    if (error)
    {
      // Fail silently as it's not a requirement
      g_error_free (error);
      error = NULL;
    }
    else if (mime_match)
    {
      _mime_regex = g_regex_new (mime_match,
                                (GRegexCompileFlags)0,
                                (GRegexMatchFlags)0,
                                &error);
      if (error)
      {
        g_warning ("Unable to compile '"MIME_PATTERN"' regex for %s: %s",
                   path,
                   error->message);
        g_error_free (error);
        error = NULL;
      }
    }

    g_free (uri_match);
    g_free (mime_match);
  }

  LoadKeyFileEntries (key_file);
    
  g_key_file_free (key_file);
}

PlaceRemote::~PlaceRemote ()
{
  std::vector<PlaceEntry*>::iterator it;
  
  for (it = _entries.begin(); it != _entries.end(); it++)
  {
    PlaceEntry *entry = static_cast<PlaceEntry *> (*it);
    delete entry;
  }

  _entries.erase (_entries.begin (), _entries.end ());

  g_free (_path);
  g_free (_dbus_name);
  g_free (_dbus_path);
  
  if (_uri_regex)
    g_regex_unref (_uri_regex);
  if (_mime_regex)
    g_regex_unref (_mime_regex);
}

std::vector<PlaceEntry *>&
PlaceRemote::GetEntries ()
{
  return _entries;
}

guint32
PlaceRemote::GetNEntries ()
{
  return _entries.size ();
}

void
PlaceRemote::LoadKeyFileEntries (GKeyFile *key_file)
{
  gchar **groups;
  gint    i = 0;

  groups = g_key_file_get_groups (key_file, NULL);

  while (groups[i])
  {
    const gchar *group = groups[i];

    if (g_str_has_prefix (group, ENTRY_PREFIX))
    {
      PlaceEntryRemote *entry = new PlaceEntryRemote (key_file, group);
      
      _entries.push_back (entry);
      entry_added.emit (entry);
    }

    i++;
  }

  g_strfreev (groups);
}
