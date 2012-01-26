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

#ifndef FAVORITE_STORE_GSETTINGS_H
#define FAVORITE_STORE_GSETTINGS_H

#include <gio/gio.h>

#include "FavoriteStore.h"
#include <UnityCore/GLibWrapper.h>

// An abstract object that facilitates getting and modifying the list of favorites
// Use GetDefault () to get the correct store for the session
namespace unity
{
namespace internal
{

class FavoriteStoreGSettings : public FavoriteStore
{
public:
  FavoriteStoreGSettings();
  FavoriteStoreGSettings(GSettingsBackend* backend);

  virtual FavoriteList const& GetFavorites();
  virtual void AddFavorite(std::string const& desktop_path, int position);
  virtual void RemoveFavorite(std::string const& desktop_path);
  virtual void MoveFavorite(std::string const& desktop_path, int position);
  void SaveFavorites(FavoriteList const& favorites, bool ignore = true);
  virtual void SetFavorites(FavoriteList const& desktop_paths);

  //Methods
  void Changed(std::string const& key);

private:
  void Init();
  void Refresh();
  void FillList(FavoriteList& list);

  FavoriteList favorites_;
  glib::Object<GSettings> settings_;
  bool ignore_signals_;
};

}
}

#endif // FAVORITE_STORE_GSETTINGS_H
