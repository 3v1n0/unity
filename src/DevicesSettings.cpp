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
* Authored by: Andrea Azzarone <aazzarone@hotmail.it>
*/

#include "DevicesSettings.h"

static DevicesSettings *_devices_settings = NULL;

DevicesSettings::DevicesSettings ()
: _settings (NULL),
  _favorites (NULL),
  _ignore_signals (false),
  _devices_option (ONLY_MOUNTED)
{
  _settings = g_settings_new ("com.canonical.Unity.Devices");
  g_signal_connect (_settings, "changed",
                    (GCallback)(DevicesSettings::Changed), this);

  Refresh ();
}

DevicesSettings::~DevicesSettings ()
{
  g_slist_foreach (_favorites, (GFunc)g_free, NULL);
  g_slist_free (_favorites);
  g_object_unref (_settings);
}

void
DevicesSettings::Refresh ()
{
  gchar **favs;
  gint i = 0;
    
  g_slist_foreach (_favorites, (GFunc)g_free, NULL);
  g_slist_free (_favorites);
  _favorites = NULL;

  favs = g_settings_get_strv (_settings, "favorites");

  while (favs[i] != NULL)
  {
    _favorites = g_slist_append (_favorites, g_strdup (favs[i]));
    
    i++;
  }
  
  g_strfreev (favs);

  changed.emit (this);
}

GSList *
DevicesSettings::GetFavorites ()
{
  return _favorites;
}

static gchar *
get_uuid (const gchar *uuid)
{
  return g_strdup (uuid);
}

void
DevicesSettings::AddFavorite (const char *uuid)
{
  int     n_total_favs;
  GSList *f;
  gint    i = 0;
  
  g_return_if_fail (uuid);
  
  n_total_favs = g_slist_length (_favorites) + 1;
  
  char *favs[n_total_favs + 1];
  favs[n_total_favs] = NULL;

  for (f = _favorites; f; f = f->next)
    favs[i++] = get_uuid ((char *)f->data);

  /* Add it to the end of the list */
  favs[i] = get_uuid (uuid);

  _ignore_signals = true;
  if (!g_settings_set_strv (_settings, "favorites", favs))
    g_warning ("Unable to add a new favorite '%s'", uuid);
  _ignore_signals = false;

  i = 0;
  while (favs[i] != NULL)
  {
    g_free (favs[i]);
    favs[i] = NULL;
    i++;
  }

  Refresh ();
}

void
DevicesSettings::RemoveFavorite (const char *uuid)
{
  int     n_total_favs;
  GSList *f;
  int     i = 0;
  bool    found = false;

  g_return_if_fail (uuid);

  n_total_favs = g_slist_length (_favorites) - 1;
  
  char *favs[n_total_favs + 1];
  favs[n_total_favs] = NULL;

  for (f = _favorites; f; f = f->next)
  {
    if (g_strcmp0 ((char *)f->data, uuid) != 0)
    {
      favs[i] = get_uuid ((char *)f->data);
      i++; 
    }
    else
    {
      found = true;
    }
  }

  if (!found)
  {
      g_warning ("Unable to remove favorite '%s': Does not exist in favorites",
                 uuid);
  }

  _ignore_signals = true;
  if (!g_settings_set_strv (_settings, "favorites", favs))
    g_warning ("Unable to remove favorite '%s'", uuid);
  _ignore_signals = false;

  i = 0;
  while (favs[i] != NULL)
  {
    g_free (favs[i]);
    favs[i] = NULL;
    i++;
  }

  Refresh ();
}


void
DevicesSettings::Changed (GSettings *settings, char *key, DevicesSettings *self)
{
  if (self->_ignore_signals)
    return;
  
  self->Refresh ();
}

DevicesSettings *
DevicesSettings::GetDefault ()
{
  if (G_UNLIKELY (!_devices_settings))
    _devices_settings = new DevicesSettings ();

  return _devices_settings;
}

void
DevicesSettings::SetDevicesOption (DevicesOption devices_option)
{
  if (devices_option == _devices_option)
    return;

  _devices_option = devices_option;

  changed.emit (this);
}
