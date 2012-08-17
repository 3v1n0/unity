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

#include "DeviceLauncherSection.h"
#include "DeviceNotificationShowerImp.h"
#include "DevicesSettings.h"
#include "FileManagerOpenerImp.h"
#include "VolumeImp.h"

namespace unity
{
namespace launcher
{

DeviceLauncherSection::DeviceLauncherSection(AbstractVolumeMonitorWrapper::Ptr volume_monitor,
                                             DevicesSettings::Ptr devices_settings)
  : monitor_(volume_monitor)
  , devices_settings_(devices_settings)
  , file_manager_opener_(new FileManagerOpenerImp)
  , device_notification_shower_(new DeviceNotificationShowerImp)
{
  monitor_->volume_added.connect(sigc::mem_fun(this, &DeviceLauncherSection::OnVolumeAdded));
  monitor_->volume_removed.connect(sigc::mem_fun(this, &DeviceLauncherSection::OnVolumeRemoved));

  device_populate_idle_.Run([this] () {
    PopulateEntries();
    return false;
  });
}

void DeviceLauncherSection::PopulateEntries()
{
  for (auto volume : monitor_->GetVolumes())
    TryToCreateAndAddIcon(volume);
}

void DeviceLauncherSection::OnVolumeAdded(glib::Object<GVolume> const& volume)
{
  TryToCreateAndAddIcon(volume);
}

void DeviceLauncherSection::TryToCreateAndAddIcon(glib::Object<GVolume> volume)
{
  if (map_.find(volume) != map_.end())
    return;

  VolumeLauncherIcon::Ptr icon(new VolumeLauncherIcon(std::make_shared<VolumeImp>(volume, file_manager_opener_, device_notification_shower_),
                                                        devices_settings_));

  map_[volume] = icon;
  IconAdded.emit(icon);
}

void DeviceLauncherSection::OnVolumeRemoved(glib::Object<GVolume> const& volume)
{
  auto volume_it = map_.find(volume);

  // Sanity check
  if (volume_it != map_.end())
  {
    volume_it->second->OnRemoved();
    map_.erase(volume_it);
  }
}

}
}
