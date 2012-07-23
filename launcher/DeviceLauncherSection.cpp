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

namespace unity
{
namespace launcher
{

DeviceLauncherSection::DeviceLauncherSection(AbstractVolumeMonitorWrapper::Ptr volume_monitor)
  : monitor_(volume_monitor)
{
  monitor_->volume_added.connect(sigc::mem_fun(this, &DeviceLauncherSection::OnVolumeAdded));
  monitor_->volume_removed.connect(sigc::mem_fun(this, &DeviceLauncherSection::OnVolumeRemoved));
  
  device_populate_idle_.Run([&] () {
    PopulateEntries();
    return false;
  });
}

void DeviceLauncherSection::PopulateEntries()
{
  for (auto volume : monitor_->GetVolumes())
  {
    // Sanity check. Avoid duplicates.
    if (map_.find(volume) != map_.end())
      continue;

    DeviceLauncherIcon::Ptr icon(new DeviceLauncherIcon(volume));

    map_[volume] = icon;
    IconAdded.emit(icon);
  }
}

/* Uses a std::map to track all the volume icons shown and not shown.
 * Keep in mind: when "volume-removed" is recevied we should erase
 * the pair (GVolume - DeviceLauncherIcon) from the std::map to avoid leaks
 */
void DeviceLauncherSection::OnVolumeAdded(glib::Object<GVolume> const& volume)
{
  // Sanity check. Avoid duplicates.
  if (map_.find(volume) != map_.end())
    return;

  DeviceLauncherIcon::Ptr icon(new DeviceLauncherIcon(volume));

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

} // namespace launcher
} // namespace unity

