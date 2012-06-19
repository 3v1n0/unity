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
 */

#include "DeviceLauncherSection.h"

namespace unity
{
namespace launcher
{

DeviceLauncherSection::DeviceLauncherSection()
  : monitor_(g_volume_monitor_get())
{
  typedef glib::Signal<void, GVolumeMonitor*, GVolume*> VolumeSignal;
  sig_manager_.Add(new VolumeSignal(monitor_, "volume-added", sigc::mem_fun(this, &DeviceLauncherSection::OnVolumeAdded)));
  sig_manager_.Add(new VolumeSignal(monitor_, "volume-removed", sigc::mem_fun(this, &DeviceLauncherSection::OnVolumeRemoved)));

  device_populate_idle_.Run([&] () {
    PopulateEntries();
    return false;
  });
}

void DeviceLauncherSection::PopulateEntries()
{
  GList* volumes = g_volume_monitor_get_volumes(monitor_);

  for (GList* v = volumes; v; v = v->next)
  {
    if (!G_IS_VOLUME(v->data))
      continue;

    // This will unref the volume, since the list entries needs that.
    // We'll keep a reference in the icon.
    glib::Object<GVolume> volume(G_VOLUME(v->data));
    DeviceLauncherIcon* icon = new DeviceLauncherIcon(volume);

    map_[volume] = icon;
    IconAdded.emit(AbstractLauncherIcon::Ptr(icon));
  }

  g_list_free(volumes);
}

/* Uses a std::map to track all the volume icons shown and not shown.
 * Keep in mind: when "volume-removed" is recevied we should erase
 * the pair (GVolume - DeviceLauncherIcon) from the std::map to avoid leaks
 */
void DeviceLauncherSection::OnVolumeAdded(GVolumeMonitor* monitor, GVolume* volume)
{
  // This just wraps the volume in a glib::Object, global ref_count is only
  // temporary changed.
  glib::Object<GVolume> gvolume(volume, glib::AddRef());
  DeviceLauncherIcon* icon = new DeviceLauncherIcon(gvolume);

  map_[gvolume] = icon;
  IconAdded.emit(AbstractLauncherIcon::Ptr(icon));
}

void DeviceLauncherSection::OnVolumeRemoved(GVolumeMonitor* monitor, GVolume* volume)
{
  auto volume_it = map_.find(volume);

  // It should not happen! Let me do the check anyway.
  if (volume_it != map_.end())
  {
    volume_it->second->OnRemoved();
    map_.erase(volume_it);
  }
}

} // namespace launcher
} // namespace unity
