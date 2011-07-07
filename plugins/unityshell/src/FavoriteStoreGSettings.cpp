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
* Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
*/

#include "FavoriteStoreGSettings.h"

#include <algorithm>
#include <iostream>

#include <gio/gdesktopappinfo.h>

#include "NuxCore/Logger.h"

#include "config.h"

/**
 * Internally the favorite store keeps the full path to the application, but
 * when saving, only the desktop file id is saved if the favorite is in one of
 * the system directories.  If the favorite isn't in one of the system
 * directories, the full path is saved in the settings.
 */

namespace unity {
namespace internal {

namespace {

nux::logging::Logger logger("unity.favorites");

const char* SETTINGS_NAME = "com.canonical.Unity.Launcher";
const char* LATEST_SETTINGS_MIGRATION = "3.2.10";

void on_settings_updated(GSettings* settings,
                         const gchar* key,
                         FavoriteStoreGSettings* self);

std::string get_basename_or_path(std::string const& desktop_path);

char* exhaustive_desktopfile_lookup(char *desktop_file);

}

FavoriteStoreGSettings::FavoriteStoreGSettings()
  : settings_(g_settings_new(SETTINGS_NAME))
  , ignore_signals_(false)
{
  Init();
}

FavoriteStoreGSettings::FavoriteStoreGSettings(GSettingsBackend *backend)
  : settings_(g_settings_new_with_backend(SETTINGS_NAME, backend))
  , ignore_signals_(false)
{
  Init();
}

void FavoriteStoreGSettings::Init()
{
  /* migrate the favorites if needed and ignore errors */
  glib::String latest_migration_update(g_settings_get_string(
                                           settings_, "favorite-migration"));
  if (latest_migration_update.Str() < LATEST_SETTINGS_MIGRATION)
  {
    glib::Error error;
    std::string cmd(PREFIXDIR);
    cmd += "/lib/unity/migrate_favorites.py";

    glib::String output;

    g_spawn_command_line_sync(cmd.c_str(),
                              output.AsOutParam(),
                              NULL,
                              NULL,
                              error.AsOutParam());
    if (error)
    {
      // Do we have a defined logging subsystem?
      std::cout << "WARNING: Unable to run the migrate favorites "
                << "tools successfully: " << error.Message()
                << ".\nThe output was:" << output.Value() << std::endl;
    }
  }

  g_signal_connect(settings_, "changed", G_CALLBACK(on_settings_updated), this);
  Refresh();
}


void FavoriteStoreGSettings::Refresh()
{
  favorites_.clear();

  gchar **favs = g_settings_get_strv (settings_, "favorites");

  for (int i = 0; favs[i] != NULL; ++i)
  {
    // We will be storing either full /path/to/desktop/files or foo.desktop id's
    if (favs[i][0] == '/')
    {
      if (g_file_test(favs[i], G_FILE_TEST_EXISTS))
      {
        favorites_.push_back(favs[i]);
      }
      else
      {
        LOG_WARNING(logger) << "Unable to load desktop file: "
                            << favs[i];
      }
    }
    else
    {
      glib::Object<GDesktopAppInfo> info(g_desktop_app_info_new(favs[i]));
      const char* filename = 0;
      if (info)
        filename = g_desktop_app_info_get_filename(info);

      if (filename)
      {
        favorites_.push_back(filename);
      }
      else
      {
        LOG_INFO(logger) << "Unable to load GDesktopAppInfo for '"
                         << favs[i] << "'";
        glib::String exhaustive_path(exhaustive_desktopfile_lookup(favs[i]));
        if (exhaustive_path.Value() == NULL)
        {
          LOG_WARNING(logger) << "Desktop file '"
                              << favs[i]
                              << "' does not exist anywhere we can find it";
        }
        else
        {
          favorites_.push_back(exhaustive_path.Str());
        }
      }
    }
  }

  g_strfreev (favs);
}

FavoriteList const&  FavoriteStoreGSettings::GetFavorites()
{
  return favorites_;
}

void FavoriteStoreGSettings::AddFavorite(std::string const& desktop_path, int position)
{
  int size = favorites_.size();
  if (desktop_path.empty() || position > size)
    return;

  if (position < 0)
  {
    // It goes on the end.
    favorites_.push_back(desktop_path);
  }
  else
  {
    FavoriteList::iterator pos = favorites_.begin();
    std::advance(pos, position);
    favorites_.insert(pos, desktop_path);
  }

  SaveFavorites(favorites_);
  Refresh();
}

void FavoriteStoreGSettings::RemoveFavorite(std::string const& desktop_path)
{
  if (desktop_path.empty() || desktop_path[0] != '/')
    return;

  FavoriteList::iterator pos = std::find(favorites_.begin(), favorites_.end(), desktop_path);
  if (pos == favorites_.end()) {
    return;
  }

  favorites_.erase(pos);
  SaveFavorites(favorites_);
  Refresh();
}

void FavoriteStoreGSettings::MoveFavorite(std::string const& desktop_path, int position)
{
  int size = favorites_.size();
  if (desktop_path.empty() || position > size)
    return;

  FavoriteList::iterator pos = std::find(favorites_.begin(), favorites_.end(), desktop_path);
  if (pos == favorites_.end()) {
    return;
  }

  favorites_.erase(pos);
  if (position < 0)
  {
    // It goes on the end.
    favorites_.push_back(desktop_path);
  }
  else
  {
    FavoriteList::iterator insert_pos = favorites_.begin();
    std::advance(insert_pos, position);
    favorites_.insert(insert_pos, desktop_path);
  }

  SaveFavorites(favorites_);
  Refresh();
}

void FavoriteStoreGSettings::SetFavorites(FavoriteList const& favorites)
{
  SaveFavorites(favorites);
  Refresh();
}

void FavoriteStoreGSettings::SaveFavorites(FavoriteList const& favorites)
{
  const int size = favorites.size();
  const char* favs[size + 1];
  favs[size] = NULL;

  int index = 0;
  // Since we don't always save the full path, we store the values we are
  // actually going to save in a different list.
  FavoriteList values;
  for (FavoriteList::const_iterator i = favorites.begin(), end = favorites.end();
       i != end; ++i, ++index)
  {
    // By using insert we get the iterator to the newly inserted string value.
    // That way we can use the c_str() method to access the const char* for
    // the string that we are going to save.  This way we know that the pointer
    // is valid for the lifetime of the favs array usage in the method call to
    // set the settings, and that we aren't referencing a temporary.
    FavoriteList::iterator iter = values.insert(values.end(), get_basename_or_path(*i));
    favs[index] = iter->c_str();
  }

  ignore_signals_ = true;
  if (!g_settings_set_strv(settings_, "favorites", favs))
  {
    LOG_WARNING(logger) << "Saving favorites failed.";
  }
  ignore_signals_ = false;
}

void FavoriteStoreGSettings::Changed(std::string const& key)
{
  if (ignore_signals_)
    return;

  LOG_DEBUG(logger) << "Changed: " << key;
}

namespace {

void on_settings_updated(GSettings* settings,
                         const gchar* key,
                         FavoriteStoreGSettings* self)
{
  if (settings and key) {
    self->Changed(key);
  }
}

std::string get_basename_or_path(std::string const& desktop_path)
{
  const gchar* const* dirs = g_get_system_data_dirs();

  /* We check to see if the desktop file belongs to one of the system data
   * directories. If so, then we store it's desktop id, otherwise we store
   * it's full path. We're clever like that.
   */
  for (int i = 0; dirs[i]; ++i)
  {
    std::string dir(dirs[i]);

    if (desktop_path.find(dir) == 0)
    {
      // Would prefer boost::filesystem here.
      glib::String basename(g_path_get_basename(desktop_path.c_str()));
      return basename.Str();
    }
  }
  return desktop_path;
}

/* If the desktop file exists, we *will* find it dang it
 * Returns null if we failed =(
 *
 * Most of this code copied from bamf - its nice to have this
 * agree with bamf at the very least
 */
char *exhaustive_desktopfile_lookup (char *desktop_file)
{
  GFile *file;
  GFileInfo *info;
  GFileEnumerator *enumerator;
  GList *dirs = NULL, *l;
  const char *env;
  char  *path;
  char  *subpath;
  char **data_dirs = NULL;
  char **data;

  env = g_getenv ("XDG_DATA_DIRS");

  if (env)
  {
    data_dirs = g_strsplit (env, ":", 0);

    for (data = data_dirs; *data; data++)
    {
      path = g_build_filename (*data, "applications", NULL);
      if (g_file_test (path, G_FILE_TEST_IS_DIR))
        dirs = g_list_prepend (dirs, path);
      else
        g_free (path);
    }
  }

  if (!g_list_find_custom (dirs, "/usr/share/applications", (GCompareFunc) g_strcmp0))
    dirs = g_list_prepend (dirs, g_strdup ("/usr/share/applications"));

  if (!g_list_find_custom (dirs, "/usr/local/share/applications", (GCompareFunc) g_strcmp0))
    dirs = g_list_prepend (dirs, g_strdup ("/usr/local/share/applications"));

  dirs = g_list_prepend (dirs, g_strdup (g_build_filename (g_get_home_dir (), ".share/applications", NULL)));

  if (data_dirs)
    g_strfreev (data_dirs);

  /* include subdirs */
  for (l = dirs; l; l = l->next)
  {
    path = (char *)l->data;

    file = g_file_new_for_path (path);

    if (!g_file_query_exists (file, NULL))
    {
      g_object_unref (file);
      continue;
    }

    enumerator = g_file_enumerate_children (file,
                                            "standard::*",
                                            G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                            NULL,
                                            NULL);

    if (!enumerator)
      continue;

    info = g_file_enumerator_next_file (enumerator, NULL, NULL);
    for (; info; info = g_file_enumerator_next_file (enumerator, NULL, NULL))
    {
      if (g_file_info_get_file_type (info) != G_FILE_TYPE_DIRECTORY)
        continue;

      subpath = g_build_filename (path, g_file_info_get_name (info), NULL);
      /* append for non-recursive recursion love */
      dirs = g_list_append (dirs, subpath);

      g_object_unref (info);
    }

    g_object_unref (enumerator);
    g_object_unref (file);
  }

  /* dirs now contains a list if lookup directories */
  /* go through the dir list and stat to check it exists */
  path = NULL;
  for (l = dirs; l; l = l->next)
  {
    path = g_build_filename ((char *)l->data, desktop_file, NULL);
    if (g_file_test (path, G_FILE_TEST_EXISTS))
      break;

    g_free (path);
    path = NULL;
  }

  g_list_free (dirs);

  return path;
}

} // anonymous namespace

} // namespace internal
} // namespace unity
