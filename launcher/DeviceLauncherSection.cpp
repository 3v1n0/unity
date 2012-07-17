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

DeviceLauncherSection::DeviceLauncherSection(VolumeMonitorWrapper::Ptr volume_monitor)
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
  auto const& volumes = monitor_->GetVolumes();

  for (auto volume : volumes)
  {
    // Sanity check. Avoid duplicates.
    if (map_.find(volume) != map_.end())
      continue;

    DeviceLauncherIcon* icon = new DeviceLauncherIcon(volume);

    map_[volume] = icon;
    IconAdded.emit(AbstractLauncherIcon::Ptr(icon));
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

  DeviceLauncherIcon* icon = new DeviceLauncherIcon(volume);
  map_[volume] = icon;
  IconAdded.emit(AbstractLauncherIcon::Ptr(icon));
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

