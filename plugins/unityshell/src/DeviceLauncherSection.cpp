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
  on_volume_added_handler_id_ = g_signal_connect(monitor_,
                                                 "volume-added",
                                                 G_CALLBACK(&DeviceLauncherSection::OnVolumeAdded),
                                                 this);

  on_volume_removed_handler_id_ = g_signal_connect(monitor_,
                                                   "volume-removed",
                                                   G_CALLBACK(&DeviceLauncherSection::OnVolumeRemoved),
                                                   this);
    
  on_mount_added_handler_id_ = g_signal_connect(monitor_,
                                                "mount-added",
                                                G_CALLBACK(&DeviceLauncherSection::OnMountAdded),
                                                this);
                                                
  on_mount_pre_unmount_handler_id_ = g_signal_connect(monitor_,
                                                      "mount-pre-unmount",
                                                      G_CALLBACK(&DeviceLauncherSection::OnMountPreUnmount),
                                                      this);

  on_device_populate_entry_id_ = g_idle_add((GSourceFunc)&DeviceLauncherSection::PopulateEntries, this);
}

DeviceLauncherSection::~DeviceLauncherSection()
{
  if (on_volume_added_handler_id_)
    g_signal_handler_disconnect((gpointer) monitor_,
                                on_volume_added_handler_id_);

  if (on_volume_removed_handler_id_)
    g_signal_handler_disconnect((gpointer) monitor_,
                                on_volume_removed_handler_id_);

  if (on_mount_added_handler_id_)
    g_signal_handler_disconnect((gpointer) monitor_,
                                on_mount_added_handler_id_);

  if (on_mount_pre_unmount_handler_id_)
    g_signal_handler_disconnect((gpointer) monitor_,
                                on_mount_pre_unmount_handler_id_);

  if (on_device_populate_entry_id_)
    g_source_remove(on_device_populate_entry_id_);
}

bool DeviceLauncherSection::PopulateEntries(DeviceLauncherSection* self)
{
  GList* volumes = g_volume_monitor_get_volumes(self->monitor_);

  for (GList* v = volumes; v; v = v->next)
  {
    if (!G_IS_VOLUME(v->data))
      continue;

    // This will unref the volume, since the list entries needs that.
    // We'll keep a reference in the icon.
    glib::Object<GVolume> volume(G_VOLUME(v->data));
    DeviceLauncherIcon* icon = new DeviceLauncherIcon(volume);

    self->map_[volume] = icon;
    self->IconAdded.emit(AbstractLauncherIcon::Ptr(icon));
  }

  g_list_free(volumes);
  self->on_device_populate_entry_id_ = 0;
  
  return false;
}

/* Uses a std::map to track all the volume icons shown and not shown.
 * Keep in mind: when "volume-removed" is recevied we should erase
 * the pair (GVolume - DeviceLauncherIcon) from the std::map to avoid leaks
 */
void DeviceLauncherSection::OnVolumeAdded(GVolumeMonitor* monitor,
                                          GVolume* volume,
                                          DeviceLauncherSection* self)
{
  // This just wraps the volume in a glib::Object, global ref_count is only
  // temporary changed.
  glib::Object<GVolume> gvolume(volume, glib::AddRef());
  DeviceLauncherIcon* icon = new DeviceLauncherIcon(gvolume);

  self->map_[gvolume] = icon;
  self->IconAdded.emit(AbstractLauncherIcon::Ptr(icon));
}

void DeviceLauncherSection::OnVolumeRemoved(GVolumeMonitor* monitor,
                                            GVolume* volume,
                                            DeviceLauncherSection* self)
{
  // It should not happen! Let me do the check anyway.
  auto volume_it = self->map_.find(volume);
  if (volume_it != self->map_.end())
  {
    volume_it->second->OnRemoved();
    self->map_.erase(volume_it);
  }
}

/* Keep in mind: we could have a GMount without a related GVolume
 * so check everything to avoid unwanted behaviors.
 */
void DeviceLauncherSection::OnMountAdded(GVolumeMonitor* monitor,
                                         GMount* mount,
                                         DeviceLauncherSection* self)
{
  glib::Object<GVolume> volume(g_mount_get_volume(mount));

  auto volume_it = self->map_.find(volume);

  if (volume_it != self->map_.end())
    volume_it->second->UpdateVisibility(1);
}

/* We don't use "mount-removed" signal since it is received after "volume-removed"
 * signal. You should read also the comment above.
 */
void DeviceLauncherSection::OnMountPreUnmount(GVolumeMonitor* monitor,
                                              GMount* mount,
                                              DeviceLauncherSection* self)
{
  glib::Object<GVolume> volume(g_mount_get_volume(mount));

  auto volume_it = self->map_.find(volume);

  if (volume_it != self->map_.end())
    volume_it->second->UpdateVisibility(0);
}

} // namespace launcher
} // namespace unity
