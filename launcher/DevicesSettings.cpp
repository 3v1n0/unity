// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010-12 Canonical Ltd
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
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 */

#include <gio/gio.h>
#include <NuxCore/Logger.h>

#include "DevicesSettings.h"
#include <UnityCore/GLibSignal.h>
#include <UnityCore/GLibWrapper.h>

namespace unity
{
namespace launcher
{
namespace
{

nux::logging::Logger logger("unity.device_settings");

const std::string SETTINGS_NAME = "com.canonical.Unity.Devices";
const std::string KEY_NAME = "favorites";

} // unnamed namespace

//
// Start private implementation
//
class DevicesSettings::Impl
{
public:
  Impl(DevicesSettings* parent)
    : parent_(parent)
    , settings_(g_settings_new(SETTINGS_NAME.c_str()))
    , ignore_signals_(false)
  {
    DownloadFavorites();
    ConnectSignals();
  }

  void ConnectSignals()
  {
    settings_changed_signal_.Connect(settings_, "changed::" + KEY_NAME,
    [this] (GSettings*, gchar*) {
      if (ignore_signals_)
        return;
  
      DownloadFavorites();
      parent_->changed.emit();
    });
  }

  void DownloadFavorites()
  {
    std::shared_ptr<gchar*> downloaded_favorites(g_settings_get_strv(settings_, KEY_NAME.c_str()), g_strfreev);

    favorites_.clear();

    auto downloaded_favorites_raw = downloaded_favorites.get();
    for (int i = 0; downloaded_favorites_raw[i]; ++i)
      favorites_.push_back(downloaded_favorites_raw[i]);
  }

  void UploadFavorites()
  {
    const int size = favorites_.size();
    const char* favorites_to_be_uploaded[size+1];

    int index = 0;
    for (auto favorite : favorites_)
      favorites_to_be_uploaded[index++] = favorite.c_str();
    favorites_to_be_uploaded[index] = nullptr;

    ignore_signals_ = true;
    if (!g_settings_set_strv(settings_, KEY_NAME.c_str(), favorites_to_be_uploaded))
    {
      LOG_WARNING(logger) << "Saving favorites failed.";
    }
    ignore_signals_ = false;
  }

  std::list<std::string> GetFavorites() const
  {
    return favorites_;
  }

  bool IsAFavoriteDevice(std::string const& uuid) const
  {
    auto begin = std::begin(favorites_);
    auto end = std::end(favorites_);
    return std::find(begin, end, uuid) == end;
  }

  void AddFavorite(std::string const& uuid)
  {
    if (uuid.empty())
      return;
  
    favorites_.push_back(uuid);

    UploadFavorites();
  }

  void RemoveFavorite(std::string const& uuid)
  {
    if (uuid.empty())
      return;

    if (!IsAFavoriteDevice(uuid))
      return;

    favorites_.remove(uuid);
    UploadFavorites();
  }

  DevicesSettings* parent_;
  glib::Object<GSettings> settings_;
  std::list<std::string> favorites_;
  bool ignore_signals_;
  glib::Signal<void, GSettings*, gchar*> settings_changed_signal_;

};

//
// End private implementation
//

DevicesSettings::DevicesSettings()
  : pimpl(new Impl(this))
{}

DevicesSettings::~DevicesSettings()
{}

std::list<std::string> DevicesSettings::GetFavorites() const
{
  return pimpl->GetFavorites();
}

bool DevicesSettings::IsAFavoriteDevice(std::string const& uuid) const
{
  return pimpl->IsAFavoriteDevice(uuid);
}

void DevicesSettings::AddFavorite(std::string const& uuid)
{
  pimpl->AddFavorite(uuid);
}

void DevicesSettings::RemoveFavorite(std::string const& uuid)
{
  pimpl->RemoveFavorite(uuid);
}

} // namespace launcher
} // namespace unity
