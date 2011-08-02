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

#include <algorithm>

namespace unity {

namespace {

const char* SETTINGS_NAME = "com.canonical.Unity.Devices";

void on_settings_updated(GSettings* settings,
                         const gchar* key,
                         DevicesSettings* self);

} // anonymous namespace

DevicesSettings& DevicesSettings::GetDefault()
{
  static DevicesSettings instance;
  return instance;
}

DevicesSettings::DevicesSettings()
  : settings_(g_settings_new(SETTINGS_NAME))
  , ignore_signals_(false)
  , devices_option_(ONLY_MOUNTED)
{

  g_signal_connect(settings_, "changed", G_CALLBACK(on_settings_updated), this);

  Refresh();
}

void DevicesSettings::Refresh()
{
  gchar** favs = g_settings_get_strv(settings_, "favorites");

  favorites_.clear();

  for (int i = 0; favs[i] != NULL; i++)
    favorites_.push_back(favs[i]);

  g_strfreev(favs);
}

void DevicesSettings::AddFavorite(std::string const& uuid)
{
  if (uuid.empty())
    return;
  
  favorites_.push_back(uuid);

  SaveFavorites(favorites_);
  Refresh();
}

void DevicesSettings::RemoveFavorite(std::string const& uuid)
{
  if (uuid.empty())
    return;

  DeviceList::iterator pos = std::find(favorites_.begin(), favorites_.end(), uuid);
  if (pos == favorites_.end())
    return;

  favorites_.erase(pos);
  SaveFavorites(favorites_);
  Refresh();
}

void DevicesSettings::SaveFavorites(DeviceList const& favorites)
{
  const int size = favorites.size();
  const char* favs[size + 1];
  favs[size] = NULL;

  int index = 0;
  for (DeviceList::const_iterator i = favorites.begin(), end = favorites.end();
       i != end; ++i, ++index)
  {
    favs[index] = i->c_str();
  }

  ignore_signals_ = true;
  if (!g_settings_set_strv(settings_, "favorites", favs))
    g_warning("Saving favorites failed.");
  ignore_signals_ = false;
}


void DevicesSettings::Changed(std::string const& key)
{
  if (ignore_signals_)
    return;
  
  Refresh();
  
  changed.emit();
}

void DevicesSettings::SetDevicesOption(DevicesOption devices_option)
{
  if (devices_option == devices_option_)
    return;

  devices_option_ = devices_option;

  changed.emit();
}

namespace {

void on_settings_updated(GSettings* settings,
                         const gchar* key,
                         DevicesSettings* self)
{
  if (settings and key) {
    self->Changed(key);
  }
}

} // anonymous namespace

} // namespace unity
