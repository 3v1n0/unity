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

DeviceLauncherSection::DeviceLauncherSection (Launcher *launcher)
: _launcher (launcher)
{
  _monitor = g_volume_monitor_get ();
  _ht = g_hash_table_new (g_direct_hash , g_direct_equal);

   _on_volume_added_handler_id = g_signal_connect (_monitor,
                                                   "volume-added",
                                                   G_CALLBACK (&DeviceLauncherSection::OnVolumeAdded),
                                                   this);

   _on_volume_removed_handler_id = g_signal_connect (_monitor,
                                                     "volume-removed",
                                                     G_CALLBACK (&DeviceLauncherSection::OnVolumeRemoved),
                                                     this);
    
   _on_mount_added_handler_id = g_signal_connect (_monitor,
                                                  "mount-added",
                                                  G_CALLBACK (&DeviceLauncherSection::OnMountAdded),
                                                  this);

   _on_device_populate_entry_id = g_idle_add ((GSourceFunc)&DeviceLauncherSection::PopulateEntries, this);
}

DeviceLauncherSection::~DeviceLauncherSection ()
{
  if (_on_volume_added_handler_id != 0)
    g_signal_handler_disconnect ((gpointer) _monitor,
                                 _on_volume_added_handler_id);

  if (_on_volume_removed_handler_id != 0)
    g_signal_handler_disconnect ((gpointer) _monitor,
                                 _on_volume_removed_handler_id);

  if (_on_mount_added_handler_id != 0)
    g_signal_handler_disconnect ((gpointer) _monitor,
                                 _on_mount_added_handler_id);

  if (_on_device_populate_entry_id)
    g_source_remove (_on_device_populate_entry_id);

  g_object_unref (_monitor);
  g_hash_table_unref (_ht);
}

bool
DeviceLauncherSection::PopulateEntries (DeviceLauncherSection *self)
{
  GList *volumes, *v;

  volumes = g_volume_monitor_get_volumes (self->_monitor);
  for (v = volumes; v; v = v->next)
  {
    GVolume *volume = (GVolume *)v->data;
    DeviceLauncherIcon *icon = new DeviceLauncherIcon (self->_launcher, volume);

    self->IconAdded.emit (icon);

    g_hash_table_insert (self->_ht, (gpointer) volume, (gpointer) icon);

    g_object_unref (volume);
  }

  g_list_free (volumes);
  
  self->_on_device_populate_entry_id = 0;

  return false;
}

void
DeviceLauncherSection::OnVolumeAdded (GVolumeMonitor        *monitor,
                                      GVolume               *volume,
                                      DeviceLauncherSection *self)
{
  DeviceLauncherIcon *icon = new DeviceLauncherIcon (self->_launcher, volume);
  
  g_hash_table_insert (self->_ht, (gpointer) volume, (gpointer) icon);  

  self->IconAdded.emit (icon);
}

void
DeviceLauncherSection::OnVolumeRemoved (GVolumeMonitor        *monitor,
                                        GVolume               *volume,
                                        DeviceLauncherSection *self)
{
    g_hash_table_remove (self->_ht, (gpointer) volume);  
}

void 
DeviceLauncherSection::OnMountAdded (GVolumeMonitor        *monitor,
                                     GMount                *mount,
                                     DeviceLauncherSection *self)
{
    GVolume *volume = g_mount_get_volume (mount);
    DeviceLauncherIcon *icon = (DeviceLauncherIcon *) g_hash_table_lookup (self->_ht, (gpointer) volume);

    if (icon)
      icon->UpdateVisibility ();
}
