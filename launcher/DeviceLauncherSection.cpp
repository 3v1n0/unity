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
#include "DeviceNotificationDisplayImp.h"
#include "DevicesSettingsImp.h"
#include "VolumeImp.h"
#include "VolumeMonitorWrapper.h"
#include "unity-shared/GnomeFileManager.h"

namespace unity
{
namespace launcher
{

DeviceLauncherSection::DeviceLauncherSection(AbstractVolumeMonitorWrapper::Ptr const& vm,
                                             DevicesSettings::Ptr const& ds,
                                             DeviceNotificationDisplay::Ptr const& notify)
  : monitor_(vm ? vm : std::make_shared<VolumeMonitorWrapper>())
  , devices_settings_(ds ? ds : std::make_shared<DevicesSettingsImp>())
  , file_manager_(GnomeFileManager::Get())
  , device_notification_display_(notify ? notify : std::make_shared<DeviceNotificationDisplayImp>())
{
  monitor_->volume_added.connect(sigc::mem_fun(this, &DeviceLauncherSection::OnVolumeAdded));
  monitor_->volume_removed.connect(sigc::mem_fun(this, &DeviceLauncherSection::OnVolumeRemoved));

  PopulateEntries();
}

void DeviceLauncherSection::PopulateEntries()
{
  for (auto const& volume : monitor_->GetVolumes())
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

  auto vol = std::make_shared<VolumeImp>(volume, file_manager_, device_notification_display_);
  VolumeLauncherIcon::Ptr icon(new VolumeLauncherIcon(vol, devices_settings_));

  map_[volume] = icon;
  icon_added.emit(icon);
}

void DeviceLauncherSection::OnVolumeRemoved(glib::Object<GVolume> const& volume)
{
  auto volume_it = map_.find(volume);

  // Sanity check
  if (volume_it != map_.end())
    map_.erase(volume_it);
}

std::vector<VolumeLauncherIcon::Ptr> DeviceLauncherSection::GetIcons() const
{
  std::vector<VolumeLauncherIcon::Ptr> icons;

  for (auto const& it : map_)
    icons.push_back(it.second);

  return icons;
}

}
}
