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

#include <gio/gdesktopappinfo.h>
#include <NuxCore/Logger.h>
#include <UnityCore/DesktopUtilities.h>

#include "FavoriteStoreGSettings.h"
#include "FavoriteStorePrivate.h"

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

void on_settings_updated(GSettings* settings,
                         const gchar* key,
                         FavoriteStoreGSettings* self);

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
  g_signal_connect(settings_, "changed", G_CALLBACK(on_settings_updated), this);
  Refresh();
}


void FavoriteStoreGSettings::Refresh()
{
  FillList(favorites_);
}

void FavoriteStoreGSettings::FillList(FavoriteList& list)
{
  list.clear();

  gchar** favs = g_settings_get_strv(settings_, "favorites");

  for (int i = 0; favs[i] != NULL; ++i)
  {
    // We will be storing either full /path/to/desktop/files or foo.desktop id's
    if (favs[i][0] == '/')
    {
      if (g_file_test(favs[i], G_FILE_TEST_EXISTS))
      {
        list.push_back(favs[i]);
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
        list.push_back(filename);
      }
      else
      {
        LOG_WARNING(logger) << "Unable to load GDesktopAppInfo for '" << favs[i] << "'";
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

void FavoriteStoreGSettings::SaveFavorites(FavoriteList const& favorites, bool ignore)
{
  const int size = favorites.size();
  const char* favs[size + 1];
  favs[size] = NULL;

  int index = 0;
  // Since we don't always save the full path, we store the values we are
  // actually going to save in a different list.
  auto system_dirs = DesktopUtilities::GetDataDirectories();
  FavoriteList values;
  for (FavoriteList::const_iterator i = favorites.begin(), end = favorites.end();
       i != end; ++i, ++index)
  {
    // By using insert we get the iterator to the newly inserted string value.
    // That way we can use the c_str() method to access the const char* for
    // the string that we are going to save.  This way we know that the pointer
    // is valid for the lifetime of the favs array usage in the method call to
    // set the settings, and that we aren't referencing a temporary.
    std::string const& desktop_id = DesktopUtilities::GetDesktopID(system_dirs, *i);
    FavoriteList::iterator iter = values.insert(values.end(), desktop_id);
    favs[index] = iter->c_str();
  }

  ignore_signals_ = ignore;
  if (!g_settings_set_strv(settings_, "favorites", favs))
  {
    LOG_WARNING(logger) << "Saving favorites failed.";
  }
  ignore_signals_ = false;
}

void FavoriteStoreGSettings::Changed(std::string const& key)
{
  if (ignore_signals_ or key != "favorites")
    return;

  FavoriteList old(favorites_);
  FillList(favorites_);

  auto newbies = impl::GetNewbies(old, favorites_);

  for (auto it : favorites_)
  {
    if (std::find(newbies.begin(), newbies.end(), it) == newbies.end())
      continue;

    std::string pos;
    bool before;

    impl::GetSignalAddedInfo(favorites_, newbies , it, pos, before);
    favorite_added.emit(it, pos, before);
  }

  for (auto it : impl::GetRemoved(old, favorites_))
  {
    favorite_removed.emit(it);
  }

  if (impl::NeedToBeReordered(old, favorites_))
    reordered.emit();

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

} // anonymous namespace

} // namespace internal
} // namespace unity
