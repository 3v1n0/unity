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
   g_signal_connect (_monitor, "volume-added",
                     G_CALLBACK (&DeviceLauncherSection::OnVolumeAdded), this);

   g_idle_add ((GSourceFunc)&DeviceLauncherSection::PopulateEntries, this);
}

DeviceLauncherSection::~DeviceLauncherSection ()
{
  g_object_unref (_monitor);
}

bool
DeviceLauncherSection::PopulateEntries (DeviceLauncherSection *self)
{
  GList *volumes, *v;

  volumes = g_volume_monitor_get_volumes (self->_monitor);
  for (v = volumes; v; v = v->next)
  {
    GVolume *volume = (GVolume *)v->data;
    GDrive  *drive = NULL;

    if ((drive = g_volume_get_drive (volume)))
    {
      DeviceLauncherIcon *icon = new DeviceLauncherIcon (self->_launcher, volume);

      self->IconAdded.emit (icon);

      g_object_unref (drive);
    }
    g_object_unref (volume);
  }

  g_list_free (volumes);

  return false;
}

void
DeviceLauncherSection::OnVolumeAdded (GVolumeMonitor        *monitor,
                                      GVolume               *volume,
                                      DeviceLauncherSection *self)
{
  DeviceLauncherIcon *icon = new DeviceLauncherIcon (self->_launcher, volume);
  self->IconAdded.emit (icon);
}
