/*
 * Copyright (C) 2012 Canonical Ltd
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

#include "VolumeMonitorWrapper.h"

namespace unity
{
namespace launcher
{

VolumeMonitorWrapper::VolumeMonitorWrapper()
  : monitor_(g_volume_monitor_get())
{
  typedef glib::Signal<void, GVolumeMonitor*, GVolume*> VolumeSignal;
  sig_manager_.Add(new VolumeSignal(monitor_, "volume-added", sigc::mem_fun(this, &VolumeMonitorWrapper::OnVolumeAdded)));
  sig_manager_.Add(new VolumeSignal(monitor_, "volume-removed", sigc::mem_fun(this, &VolumeMonitorWrapper::OnVolumeRemoved)));
}

VolumeMonitorWrapper::VolumeList VolumeMonitorWrapper::GetVolumes()
{
  VolumeList ret;
  auto volumes = std::shared_ptr<GList>(g_volume_monitor_get_volumes(monitor_), g_list_free);

  for (GList* v = volumes.get(); v; v = v->next)
  {
    if (!G_IS_VOLUME(v->data))
      continue;

    // This will unref the volume, since the list entries needs that.
    // We'll keep a reference in the list.
    glib::Object<GVolume> volume(G_VOLUME(v->data));
    ret.push_back(volume);
  }

  return ret;
}

void VolumeMonitorWrapper::OnVolumeAdded(GVolumeMonitor* monitor, GVolume* volume)
{
  glib::Object<GVolume> gvolume(volume, glib::AddRef());
  volume_added.emit(gvolume);
}

void VolumeMonitorWrapper::OnVolumeRemoved(GVolumeMonitor* monitor, GVolume* volume)
{
  glib::Object<GVolume> gvolume(volume, glib::AddRef());
  volume_removed.emit(gvolume);
}

} // namespace launcher
} // namespace unity

