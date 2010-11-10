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

// An abstract object that facilitates getting and modifying the list of favorites
// Use GetDefault () to get the correct store for the session

class FavoriteStoreGSettings : public FavoriteStore
{
public:

  FavoriteStoreGSettings ();
  FavoriteStoreGSettings (GSettingsBackend *backend);
  ~FavoriteStoreGSettings ();

  //Methods
  virtual GSList * GetFavorites ();
  virtual void     AddFavorite    (const char *desktop_path, guint32 position);
  virtual void     RemoveFavorite (const char *desktop_path);
  virtual void     MoveFavorite   (const char *desktop_path, guint32 position);

private:
  void Refresh ();

  GSList    *m_favorites;
  GSettings *m_settings;
};

#endif // FAVORITE_STORE_GSETTINGS_H
