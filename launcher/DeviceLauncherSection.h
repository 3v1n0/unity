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

#include <gio/gio.h>
#include <sigc++/sigc++.h>
#include <sigc++/signal.h>
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/GLibSignal.h>

#include "DeviceLauncherIcon.h"

class Launcher;
class LauncherIcon;

namespace unity
{
namespace launcher
{

class DeviceLauncherSection : public sigc::trackable
{
public:
  DeviceLauncherSection();
  ~DeviceLauncherSection();

  sigc::signal<void, AbstractLauncherIcon::Ptr> IconAdded;

private:
  void PopulateEntries();

  void OnVolumeAdded(GVolumeMonitor* monitor, GVolume* volume);
  void OnVolumeRemoved(GVolumeMonitor* monitor, GVolume* volume);
  void OnMountAdded(GVolumeMonitor* monitor, GMount* mount);
  void OnMountPreUnmount(GVolumeMonitor* monitor, GMount* mount);

private:
  glib::SignalManager sig_manager_;
  glib::Object<GVolumeMonitor> monitor_;
  std::map<GVolume*, DeviceLauncherIcon*> map_;
  gulong on_device_populate_entry_id_;
};

}
} // namespace unity

#endif // _DEVICE_LAUNCHER_SECTION_H_
