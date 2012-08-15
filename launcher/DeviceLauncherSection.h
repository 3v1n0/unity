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

#include <UnityCore/GLibSource.h>

#include "AbstractVolumeMonitorWrapper.h"
#include "VolumeLauncherIcon.h"

namespace unity
{
namespace launcher
{

class DevicesSettings;
class DeviceNotificationShower;
class FileManagerOpener;

class DeviceLauncherSection : public sigc::trackable
{
public:
  DeviceLauncherSection(AbstractVolumeMonitorWrapper::Ptr volume_monitor,
  	                    std::shared_ptr<DevicesSettings> devices_settings);

  sigc::signal<void, AbstractLauncherIcon::Ptr> IconAdded;

private:
  void PopulateEntries();
  void OnVolumeAdded(glib::Object<GVolume> const& volume);
  void OnVolumeRemoved(glib::Object<GVolume> const& volume);

  std::map<GVolume*, VolumeLauncherIcon::Ptr> map_;
  AbstractVolumeMonitorWrapper::Ptr monitor_;
  std::shared_ptr<DevicesSettings> devices_settings_;
  std::shared_ptr<FileManagerOpener> file_manager_opener_;
  std::shared_ptr<DeviceNotificationShower> device_notification_shower_;

  glib::Idle device_populate_idle_;
};

} // namespace launcher
} // namespace unity

#endif // UNITYSHELL_DEVICE_LAUNCHER_SECTION_H
