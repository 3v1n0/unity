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

#ifndef _DEVICE_LAUNCHER_SECTION_H_
#define _DEVICE_LAUNCHER_SECTION_H_

#include <map>

#include <sigc++/sigc++.h>
#include <sigc++/signal.h>
#include <gio/gio.h>

#include "Launcher.h"
#include "LauncherIcon.h"
#include "DeviceLauncherIcon.h"
#include "GLibWrapper.h"

namespace unity {

class DeviceLauncherSection : public sigc::trackable
{
public:
  DeviceLauncherSection(Launcher* launcher);
  ~DeviceLauncherSection();

  sigc::signal<void, LauncherIcon* > IconAdded;

private:
  static bool PopulateEntries(DeviceLauncherSection* self);
  static void OnVolumeAdded(GVolumeMonitor* monitor,
                            GVolume* volume,
                            DeviceLauncherSection* self);
  static void OnVolumeRemoved(GVolumeMonitor* monitor,
                              GVolume* volume,
                              DeviceLauncherSection* self);
  static void OnMountAdded(GVolumeMonitor* monitor,
                           GMount* mount,
                           DeviceLauncherSection* self);
  static void OnMountPreUnmount(GVolumeMonitor* monitor,
                                GMount* mount,
                                DeviceLauncherSection* self);

private:
  Launcher* launcher_;
  glib::Object<GVolumeMonitor> monitor_;
  std::map<GVolume* , DeviceLauncherIcon* > map_;
  gulong on_volume_added_handler_id_;
  gulong on_volume_removed_handler_id_;
  gulong on_mount_added_handler_id_;
  gulong on_mount_pre_unmount_handler_id_;
  gulong on_device_populate_entry_id_;
};

} // namespace unity

#endif // _DEVICE_LAUNCHER_SECTION_H_
