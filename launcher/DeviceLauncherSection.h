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
 *              Andrea Azzarone <andrea.azzarone@canonical.com>
 */

#ifndef UNITYSHELL_DEVICE_LAUNCHER_SECTION_H
#define UNITYSHELL_DEVICE_LAUNCHER_SECTION_H

#include <map>
#include <memory>

#include "AbstractVolumeMonitorWrapper.h"
#include "DevicesSettings.h"
#include "DeviceNotificationDisplay.h"
#include "VolumeLauncherIcon.h"
#include "unity-shared/FileManager.h"

namespace unity
{
namespace launcher
{

class DeviceLauncherSection : public sigc::trackable
{
public:
  DeviceLauncherSection(AbstractVolumeMonitorWrapper::Ptr volume_monitor,
                        DevicesSettings::Ptr devices_settings);

  std::vector<VolumeLauncherIcon::Ptr> GetIcons() const;

  sigc::signal<void, AbstractLauncherIcon::Ptr const&> icon_added;

private:
  void PopulateEntries();
  void OnVolumeAdded(glib::Object<GVolume> const& volume);
  void OnVolumeRemoved(glib::Object<GVolume> const& volume);
  void TryToCreateAndAddIcon(glib::Object<GVolume> volume);

  std::map<GVolume*, VolumeLauncherIcon::Ptr> map_;
  AbstractVolumeMonitorWrapper::Ptr monitor_;
  DevicesSettings::Ptr devices_settings_;
  FileManager::Ptr file_manager_;
  DeviceNotificationDisplay::Ptr device_notification_display_;
};

}
}

#endif
