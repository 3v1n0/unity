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

#include <list>

#include <gio/gio.h>
#include <NuxCore/Logger.h>

#include "DevicesSettingsImp.h"
#include <UnityCore/GLibSignal.h>
#include <UnityCore/GLibWrapper.h>

namespace unity
{
namespace launcher
{
DECLARE_LOGGER(logger, "unity.device.settings");
namespace
{
const std::string SETTINGS_NAME = "com.canonical.Unity.Devices";
const std::string KEY_NAME = "blacklist";

}

//
// Start private implementation
//
class DevicesSettingsImp::Impl
{
public:
  Impl(DevicesSettingsImp* parent)
    : parent_(parent)
    , settings_(g_settings_new(SETTINGS_NAME.c_str()))
  {
    DownloadBlacklist();
    ConnectSignals();
  }

  void ConnectSignals()
  {
    settings_changed_signal_.Connect(settings_, "changed::" + KEY_NAME, [this] (GSettings*, gchar*) {
      DownloadBlacklist();
      parent_->changed.emit();
    });
  }

  void DownloadBlacklist()
  {
    std::shared_ptr<gchar*> downloaded_blacklist(g_settings_get_strv(settings_, KEY_NAME.c_str()), g_strfreev);

    blacklist_.clear();

    auto downloaded_blacklist_raw = downloaded_blacklist.get();
    for (int i = 0; downloaded_blacklist_raw[i]; ++i)
      blacklist_.push_back(downloaded_blacklist_raw[i]);
  }

  void UploadBlacklist()
  {
    const int size = blacklist_.size();
    const char* blacklist_to_be_uploaded[size+1];

    int index = 0;
    for (auto item : blacklist_)
      blacklist_to_be_uploaded[index++] = item.c_str();
    blacklist_to_be_uploaded[index] = nullptr;

    if (!g_settings_set_strv(settings_, KEY_NAME.c_str(), blacklist_to_be_uploaded))
    {
      LOG_WARNING(logger) << "Saving blacklist failed.";
    }
  }

  bool IsABlacklistedDevice(std::string const& uuid) const
  {
    auto begin = std::begin(blacklist_);
    auto end = std::end(blacklist_);
    return std::find(begin, end, uuid) != end;
  }

  void TryToBlacklist(std::string const& uuid)
  {
    if (uuid.empty() || IsABlacklistedDevice(uuid))
      return;

    blacklist_.push_back(uuid);
    UploadBlacklist();
  }

  void TryToUnblacklist(std::string const& uuid)
  {
    if (uuid.empty() || !IsABlacklistedDevice(uuid))
      return;

    blacklist_.remove(uuid);
    UploadBlacklist();
  }

  DevicesSettingsImp* parent_;
  glib::Object<GSettings> settings_;
  std::list<std::string> blacklist_;
  glib::Signal<void, GSettings*, gchar*> settings_changed_signal_;

};

//
// End private implementation
//

DevicesSettingsImp::DevicesSettingsImp()
  : pimpl(new Impl(this))
{}

DevicesSettingsImp::~DevicesSettingsImp()
{}

bool DevicesSettingsImp::IsABlacklistedDevice(std::string const& uuid) const
{
  return pimpl->IsABlacklistedDevice(uuid);
}

void DevicesSettingsImp::TryToBlacklist(std::string const& uuid)
{
  pimpl->TryToBlacklist(uuid);
}

void DevicesSettingsImp::TryToUnblacklist(std::string const& uuid)
{
  pimpl->TryToUnblacklist(uuid);
}

} // namespace launcher
} // namespace unity
