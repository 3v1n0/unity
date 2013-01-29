// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2010-2012 Canonical Ltd
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
*              Marco Trevisan <marco.trevisan@canonical.com>
*/

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
DECLARE_LOGGER(logger, "unity.favorite.store.gsettings");

namespace
{
const std::string SETTINGS_NAME = "com.canonical.Unity.Launcher";
const std::string SETTINGS_KEY = "favorites";
}

FavoriteStoreGSettings::FavoriteStoreGSettings()
  : ignore_signals_(false)
  , settings_(g_settings_new(SETTINGS_NAME.c_str()))
{
  favorites_changed_.Connect(settings_, "changed::"+SETTINGS_KEY, [&] (GSettings*, gchar*)
  {
    Changed();
  });

  Refresh();
}

void FavoriteStoreGSettings::Refresh()
{
  FillList();
}

void FavoriteStoreGSettings::FillList()
{
  favorites_.clear();
  std::unique_ptr<gchar*[], void(*)(gchar**)> favs(g_settings_get_strv(settings_, SETTINGS_KEY.c_str()), g_strfreev);

  for (int i = 0; favs[i]; ++i)
  {
    std::string const& fav = ParseFavoriteFromUri(favs[i]);

    if (!fav.empty())
      favorites_.push_back(fav);
  }
}

FavoriteList const& FavoriteStoreGSettings::GetFavorites() const
{
  return favorites_;
}

void FavoriteStoreGSettings::AddFavorite(std::string const& icon_uri, int position)
{
  std::string const& fav = ParseFavoriteFromUri(icon_uri);

  if (fav.empty() || position > static_cast<int>(favorites_.size()))
    return;

  if (position < 0)
  {
    // It goes on the end.
    favorites_.push_back(fav);
  }
  else
  {
    FavoriteList::iterator pos = favorites_.begin();
    std::advance(pos, position);
    favorites_.insert(pos, fav);
  }

  SaveFavorites(favorites_);
  Refresh();
}

void FavoriteStoreGSettings::RemoveFavorite(std::string const& icon_uri)
{
  std::string const& fav = ParseFavoriteFromUri(icon_uri);

  if (fav.empty())
    return;

  FavoriteList::iterator pos = std::find(favorites_.begin(), favorites_.end(), fav);
  if (pos == favorites_.end())
    return;

  favorites_.erase(pos);
  SaveFavorites(favorites_);
  Refresh();
}

void FavoriteStoreGSettings::MoveFavorite(std::string const& icon_uri, int position)
{
  std::string const& fav = ParseFavoriteFromUri(icon_uri);

  if (fav.empty() || position > static_cast<int>(favorites_.size()))
    return;

  FavoriteList::iterator pos = std::find(favorites_.begin(), favorites_.end(), fav);
  if (pos == favorites_.end())
    return;

  favorites_.erase(pos);
  if (position < 0)
  {
    // It goes on the end.
    favorites_.push_back(fav);
  }
  else
  {
    FavoriteList::iterator insert_pos = favorites_.begin();
    std::advance(insert_pos, position);
    favorites_.insert(insert_pos, fav);
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

  // Since we don't always save the full path, we store the values we are
  // actually going to save in a different list.
  FavoriteList values;
  int index = 0;

  for (auto const& fav_uri : favorites)
  {
    std::string const& fav = ParseFavoriteFromUri(fav_uri);
    if (fav.empty())
    {
      LOG_WARNING(logger) << "Impossible to add favorite '" << fav_uri << "' to store";
      continue;
    }

    // By using insert we get the iterator to the newly inserted string value.
    // That way we can use the c_str() method to access the const char* for
    // the string that we are going to save.  This way we know that the pointer
    // is valid for the lifetime of the favs array usage in the method call to
    // set the settings, and that we aren't referencing a temporary.
    FavoriteList::iterator iter = values.insert(values.end(), fav);
    favs[index] = iter->c_str();
    ++index;
  }

  for (int i = index; i <= size; ++i)
    favs[i] = nullptr;

  ignore_signals_ = ignore;
  if (!g_settings_set_strv(settings_, SETTINGS_KEY.c_str(), favs))
  {
    LOG_WARNING(logger) << "Saving favorites failed.";
  }
  ignore_signals_ = false;
}

void FavoriteStoreGSettings::Changed()
{
  if (ignore_signals_)
    return;

  FavoriteList old(favorites_);
  FillList();

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

bool FavoriteStoreGSettings::IsFavorite(std::string const& icon_uri) const
{
  return std::find(favorites_.begin(), favorites_.end(), icon_uri) != favorites_.end();
}

int FavoriteStoreGSettings::FavoritePosition(std::string const& icon_uri) const
{
  int index = 0;

  for (auto const& fav : favorites_)
  {
    if (fav == icon_uri)
      return index;

    ++index;
  }

  return -1;
}

} // namespace internal
} // namespace unity
