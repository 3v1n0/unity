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

#include "DeviceLauncherIcon.h"
#include "DeviceLauncherSection.h"

namespace unity
{

DeviceLauncherSection::DeviceLauncherSection(Launcher *launcher)
	: launcher_(launcher)
	, monitor_(g_volume_monitor_get())
{
	on_volume_added_handler_id_ = g_signal_connect(monitor_.RawPtr(),
                                                 "volume-added",
                                                 G_CALLBACK(&DeviceLauncherSection::OnVolumeAdded),
                                                 this);

  on_volume_removed_handler_id_ = g_signal_connect(monitor_.RawPtr(),
                                                   "volume-removed",
                                                   G_CALLBACK(&DeviceLauncherSection::OnVolumeRemoved),
                                                   this);
    
  on_mount_added_handler_id_ = g_signal_connect(monitor_.RawPtr(),
                                                "mount-added",
                                                G_CALLBACK(&DeviceLauncherSection::OnMountAdded),
                                                this);

  on_device_populate_entry_id_ = g_idle_add((GSourceFunc)&DeviceLauncherSection::PopulateEntries, this);
}

DeviceLauncherSection::~DeviceLauncherSection()
{
  if (on_volume_added_handler_id_)
    g_signal_handler_disconnect((gpointer) monitor_.RawPtr(),
                                on_volume_added_handler_id_);

  if (on_volume_removed_handler_id_)
    g_signal_handler_disconnect((gpointer) monitor_.RawPtr(),
                                on_volume_removed_handler_id_);

  if (on_mount_added_handler_id_)
    g_signal_handler_disconnect((gpointer) monitor_.RawPtr(),
                                on_mount_added_handler_id_);

  if (on_device_populate_entry_id_)
    g_source_remove(on_device_populate_entry_id_);
}

bool DeviceLauncherSection::PopulateEntries(DeviceLauncherSection *self)
{
  GList *volumes = g_volume_monitor_get_volumes(self->monitor_.RawPtr());

  for (GList *v = volumes; v; v = v->next)
  {
    glib::Object<GVolume> volume((GVolume *)v->data);
    DeviceLauncherIcon *icon = new DeviceLauncherIcon(self->launcher_, volume.RawPtr());

		self->map_[volume.RawPtr()] = icon;
    self->IconAdded.emit(icon);
  }

  g_list_free(volumes);
  
  self->on_device_populate_entry_id_ = 0;

  return false;
}

void DeviceLauncherSection::OnVolumeAdded(GVolumeMonitor *monitor,
                                      		GVolume *volume,
                                      		DeviceLauncherSection *self)
{
  DeviceLauncherIcon *icon = new DeviceLauncherIcon(self->launcher_, volume);
  
  self->map_[volume] = icon;
  self->IconAdded.emit(icon);
}

void DeviceLauncherSection::OnVolumeRemoved(GVolumeMonitor *monitor,
                                        		GVolume *volume,
                                        		DeviceLauncherSection *self)
{
	// It should not happen! Let me do the check anyway.
  if (self->map_.find(volume) != self->map_.end())
	{	
		self->map_[volume]->OnRemoved();
		self->map_.erase(volume);
	}
}

void DeviceLauncherSection::OnMountAdded(GVolumeMonitor *monitor,
                                     		 GMount *mount,
                                     		 DeviceLauncherSection *self)
{
		std::map<GVolume *, DeviceLauncherIcon *>::iterator it;
		glib::Object<GVolume> volume(g_mount_get_volume(mount));

		it = self->map_.find(volume.RawPtr());

    if (it != self->map_.end())
      it->second->UpdateVisibility();
}

} // namespace unity
