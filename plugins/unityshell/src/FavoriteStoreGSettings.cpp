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

#include <NuxCore/Logger.h>

#include "config.h"

/**
 * Internally the favorite store keeps the full path to the application, but
 * when saving, only the desktop file id is saved if the favorite is in one of
 * the system directories.  If the favorite isn't in one of the system
 * directories, the full path is saved in the settings.
 */

namespace unity
{
namespace internal
{

namespace
{

nux::logging::Logger logger("unity.favorites");

const char* SETTINGS_NAME = "com.canonical.Unity.Launcher";
const char* LATEST_SETTINGS_MIGRATION = "3.2.10";

void on_settings_updated(GSettings* settings,
                         const gchar* key,
                         FavoriteStoreGSettings* self);

std::string get_basename_or_path(std::string const& desktop_path);

}

FavoriteStoreGSettings::FavoriteStoreGSettings()
  : settings_(g_settings_new(SETTINGS_NAME))
  , ignore_signals_(false)
{
  Init();
}

FavoriteStoreGSettings::FavoriteStoreGSettings(GSettingsBackend* backend)
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
                              &output,
                              NULL,
                              NULL,
                              &error);
    if (error)
    {
      LOG_WARN(logger) << "WARNING: Unable to run the migrate favorites "
                       << "tools successfully: " << error
                       << ".\n\tThe output was:" << output;
    }
  }

  g_signal_connect(settings_, "changed", G_CALLBACK(on_settings_updated), this);
  Refresh();
}


void FavoriteStoreGSettings::Refresh()
{
  favorites_.clear();

  gchar** favs = g_settings_get_strv(settings_, "favorites");

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
        LOG_WARNING(logger) << "Unable to load GDesktopAppInfo for '"
                         << favs[i] << "'";
      }
    }
  }

  g_strfreev(favs);
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
  if (pos == favorites_.end())
  {
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
  if (pos == favorites_.end())
  {
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

namespace
{

void on_settings_updated(GSettings* settings,
                         const gchar* key,
                         FavoriteStoreGSettings* self)
{
  if (settings and key)
  {
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
    if (dir.at(dir.length()-1) == G_DIR_SEPARATOR)
      dir.append("applications/");
    else
      dir.append("/applications/");

    if (desktop_path.find(dir) == 0)
    {
      // if we are in a subdirectory of system patch, the store name should
      // be subdir-filename.desktop
      std::string desktop_suffix = desktop_path.substr(dir.size());
      std::replace(desktop_suffix.begin(), desktop_suffix.end(), G_DIR_SEPARATOR, '-');
      return desktop_suffix;
    }
  }
  return desktop_path;
}


} // anonymous namespace

} // namespace internal
} // namespace unity
