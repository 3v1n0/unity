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

#include "config.h"

#include <gio/gdesktopappinfo.h>

#include "FavoriteStoreGSettings.h"

#define SETTINGS_NAME "com.canonical.Unity.Launcher"

#define LATEST_SETTINGS_MIGRATION "3.2.0"

static void on_settings_updated (GSettings *settings, const gchar *key, FavoriteStoreGSettings *self);

FavoriteStoreGSettings::FavoriteStoreGSettings ()
{
  m_settings = g_settings_new (SETTINGS_NAME);

  Init ();
}

FavoriteStoreGSettings::FavoriteStoreGSettings (GSettingsBackend *backend)
{
  m_settings = g_settings_new_with_backend (SETTINGS_NAME, backend);

  Init ();
}

void
FavoriteStoreGSettings::Init ()
{
  gchar *latest_migration_update;

  m_favorites = NULL;
  m_ignore_signals = false;

  /* migrate the favorites if needed and ignore errors */
  latest_migration_update = g_settings_get_string (m_settings, "favorite-migration");
  if (g_strcmp0 (latest_migration_update, LATEST_SETTINGS_MIGRATION) < 0)
    {
      GError *error = NULL;
      char *cmd = g_strdup_printf ("%s/lib/unity/migrate_favorites.py", PREFIXDIR);
      char *output;
      
      g_spawn_command_line_sync (cmd, &output, NULL, NULL, &error);
      if (error)
        {
          printf ("WARNING: Unable to run the migrate favorites tools successfully: %s.\nThe output was:%s\n", error->message,output);
          g_error_free (error);
        }
      g_free (output);
      g_free (cmd);
    }
  g_free (latest_migration_update);

  g_signal_connect (m_settings, "changed",
                    G_CALLBACK (on_settings_updated), this);

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
  m_favorites = NULL;

  favs = g_settings_get_strv (m_settings, "favorites");

  while (favs[i] != NULL)
    {
      /*
       * We will be storing either full /path/to/desktop/files or foo.desktop id's
       */
      if (favs[i][0] == '/')
        {
          if (g_file_test (favs[i], G_FILE_TEST_EXISTS))
            {
              m_favorites = g_slist_append (m_favorites, g_strdup (favs[i]));
            }
          else
            {
              g_warning ("Unable to load desktop file: %s", favs[i]);
            }
        }
      else
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

          g_object_unref (info);
        }

      i++;
    }

  g_strfreev (favs);
}

GSList *
FavoriteStoreGSettings::GetFavorites ()
{
  return m_favorites;
}

static gchar *
get_basename_or_path (const gchar *desktop_path)
{
  const gchar * const * dirs;
  const gchar * dir;
  gint          i = 0;

  dirs = g_get_system_data_dirs ();

  /* We check to see if the desktop file belongs to one of the system data
   * directories. If so, then we store it's desktop id, otherwise we store
   * it's full path. We're clever like that.
   */
  while ((dir = dirs[i]))
    {
      if (g_str_has_prefix (desktop_path, dir))
        {
          return g_path_get_basename (desktop_path);
        }
      i++;
    }

  return g_strdup (desktop_path);
}

void
FavoriteStoreGSettings::AddFavorite (const char *desktop_path,
                                     gint        position)
{
  int     n_total_favs;
  GSList *f;
  gint    i = 0;

  g_return_if_fail (desktop_path);
  g_return_if_fail (position < (gint)g_slist_length (m_favorites));
  
  n_total_favs = g_slist_length (m_favorites) + 1;
  
  char *favs[n_total_favs + 1];
  favs[n_total_favs] = NULL;

  for (f = m_favorites; f; f = f->next)
    {
      if (i == position)
        {
          favs[i] = get_basename_or_path (desktop_path);
          i++;
        }
      
      favs[i] = get_basename_or_path ((char *)f->data);

      i++;
    }

  /* Add it to the end of the list */
  if (position == -1)
    {
      favs[i] = get_basename_or_path (desktop_path);
    }

  m_ignore_signals = true;
  if (!g_settings_set_strv (m_settings, "favorites", favs))
    g_warning ("Unable to add a new favorite '%s' at position '%u'", desktop_path, position);
  m_ignore_signals = false;

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
FavoriteStoreGSettings::RemoveFavorite (const char *desktop_path)
{
  int     n_total_favs;
  GSList *f;
  int     i = 0;
  bool    found = false;

  g_return_if_fail (desktop_path);
  g_return_if_fail (desktop_path[0] == '/');

  n_total_favs = g_slist_length (m_favorites) - 1;
  
  char *favs[n_total_favs + 1];
  favs[n_total_favs] = NULL;

  for (f = m_favorites; f; f = f->next)
    {
      if (g_strcmp0 ((char *)f->data, desktop_path) != 0)
        {
          favs[i] = get_basename_or_path ((char *)f->data);
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
                 desktop_path);
    }

  m_ignore_signals = true;
  if (!g_settings_set_strv (m_settings, "favorites", favs))
    g_warning ("Unable to remove favorite '%s'", desktop_path);
  m_ignore_signals = false;

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
FavoriteStoreGSettings::MoveFavorite (const char *desktop_path,
                                      gint        position)
{
  int     n_total_favs;
  GSList *f;
  gint    i = 0;

  g_return_if_fail (desktop_path);
  g_return_if_fail (position < (gint)g_slist_length (m_favorites));
  
  n_total_favs = g_slist_length (m_favorites);
  
  char *favs[n_total_favs + 1];
  favs[n_total_favs] = NULL;

  for (f = m_favorites; f; f = f->next)
    {
      if (i == position)
        {
          favs[i] = get_basename_or_path (desktop_path);
          i++;
        }

      if (g_strcmp0 (desktop_path, (char *)f->data) != 0)
        {
          favs[i] = get_basename_or_path ((char *)f->data);
          i++;
        }
    }

  /* Add it to the end of the list */
  if (position == -1)
    {
      favs[i] = get_basename_or_path (desktop_path);
      i++;
    }
  favs[i] = NULL;

  m_ignore_signals = true;
  if (!g_settings_set_strv (m_settings, "favorites", favs))
    g_warning ("Unable to add a new favorite '%s' at position '%u'", desktop_path, position);
  m_ignore_signals = false;

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
FavoriteStoreGSettings::Changed (const gchar *key)
{
  if (m_ignore_signals)
    return;

  g_print ("Changed: %s\n", key);
}

/*
 * These are just callbacks chaining to real class functions
 */
static void
on_settings_updated (GSettings *settings, const gchar *key, FavoriteStoreGSettings *self)
{
  g_return_if_fail (key != NULL);
  g_return_if_fail (self != NULL);

  self->Changed (key);
}
