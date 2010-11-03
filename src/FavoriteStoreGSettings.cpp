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

#include <gio/gdesktopappinfo.h>

#include "FavoriteStoreGSettings.h"

FavoriteStoreGSettings::FavoriteStoreGSettings ()
{
  m_settings = g_settings_new ("com.canonical.Unity.Launcher");
  m_favorites = NULL;

  Refresh ();
}
 

FavoriteStoreGSettings::~FavoriteStoreGSettings ()
{
  g_slist_foreach (m_favorites, (GFunc)g_free, NULL);
  g_slist_free (m_favorites);
  g_object_unref (m_settings);
}

void
FavoriteStoreGSettings::Refresh ()
{
  int     i = 0;
  gchar **favs;

  g_slist_foreach (m_favorites, (GFunc)g_free, NULL);
  g_slist_free (m_favorites);

  favs = g_settings_get_strv (m_settings, "favorites");

  while (favs[i] != NULL)
    {
      GDesktopAppInfo *info;

      info = g_desktop_app_info_new (favs[i]);
      
      if (info == NULL || g_desktop_app_info_get_filename (info) == NULL)
        {
          g_warning ("Unable to load GDesktopAppInfo for '%s'", favs[i]);

          i++;
          continue;
        }

      m_favorites = g_slist_append (m_favorites, g_strdup (g_desktop_app_info_get_filename (info)));
      i++;

      g_object_unref (info);
    }

  g_strfreev (favs);
}

GSList *
FavoriteStoreGSettings::GetFavorites ()
{
  return m_favorites;
}

void
FavoriteStoreGSettings::AddFavorite (const char *desktop_path,
                                     guint32     position)
{
  int     n_total_favs;
  GSList *f;
  guint32 i = 0;

  g_return_if_fail (desktop_path);

  n_total_favs = g_slist_length (m_favorites) + 1;
  
  char *favs[n_total_favs + 1];
  favs[n_total_favs] = NULL;

  for (f = m_favorites; f; f = f->next)
    {
      if (i == position)
        {
          favs[i] = g_path_get_basename (desktop_path);
          i++;
        }
      
      favs[i] = g_path_get_basename ((char *)f->data);

      i++;
    }

  if (!g_settings_set_strv (m_settings, "favorites", favs))
    g_warning ("Unable to add a new favorite");

  i = 0;
  while (favs[i] != NULL)
    {
      g_free (favs[i]);
      favs[i] = NULL;
      i++;
    }
}
 
void
FavoriteStoreGSettings::RemoveFavorite (const char *desktop_path)
{
  int     n_total_favs;
  GSList *f;
  int     i = 0;

  g_return_if_fail (desktop_path);

  n_total_favs = g_slist_length (m_favorites) - 1;
  
  char *favs[n_total_favs + 1];
  favs[n_total_favs] = NULL;

  for (f = m_favorites; f; f = f->next)
    {
      if (g_strcmp0 ((char *)f->data, desktop_path))
        {
          favs[i] = g_path_get_basename ((char *)f->data);
          i++;
        }
    }

  if (!g_settings_set_strv (m_settings, "favorites", favs))
    g_warning ("Unable to add a new favorite");

  i = 0;
  while (favs[i] != NULL)
    {
      g_free (favs[i]);
      favs[i] = NULL;
      i++;
    }
}

void
FavoriteStoreGSettings::MoveFavorite (const char *desktop_path,
                                      guint32     position)
{
}
