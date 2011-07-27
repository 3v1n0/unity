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

#ifndef DEVICES_SETTINGS_H
#define DEVICES_SETTINGS_H

#include <list>
#include <string>

#include <gio/gio.h>
#include <boost/utility.hpp>
#include <sigc++/sigc++.h>
#include <UnityCore/GLibWrapper.h>

namespace unity {

typedef std::list<std::string> DeviceList;

class DevicesSettings : boost::noncopyable
{
public:
  typedef enum
  {
    NEVER = 0,
    ONLY_MOUNTED,
    ALWAYS

  } DevicesOption;

  DevicesSettings();
  
  static DevicesSettings& GetDefault();

  void SetDevicesOption(DevicesOption devices_option);
  DevicesOption GetDevicesOption() { return devices_option_; };

  DeviceList const& GetFavorites() { return favorites_; };
  void AddFavorite(std::string const& uuid);
  void RemoveFavorite(std::string const& uuid);

  void Changed(std::string const& key);
  
  // Signals
  sigc::signal<void> changed;
  
private:
  void Refresh();
  void SaveFavorites(DeviceList const& favorites);

  glib::Object<GSettings> settings_;
  DeviceList favorites_;
  bool ignore_signals_;
  DevicesOption devices_option_;
};

} // namespace unity

#endif // DEVICES_SETTINGS_H
