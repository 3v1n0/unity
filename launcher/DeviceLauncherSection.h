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

#ifndef _DEVICE_LAUNCHER_SECTION_H_
#define _DEVICE_LAUNCHER_SECTION_H_

#include <map>

#include <UnityCore/GLibSource.h>

#include "DeviceLauncherIcon.h"
#include "AbstractVolumeMonitorWrapper.h"

namespace unity
{
namespace launcher
{

class DeviceLauncherSection : public sigc::trackable
{
public:
  DeviceLauncherSection(AbstractVolumeMonitorWrapper::Ptr volume_monitor);

  sigc::signal<void, AbstractLauncherIcon::Ptr> IconAdded;

private:
  void PopulateEntries();
  void OnVolumeAdded(glib::Object<GVolume> const& volume);
  void OnVolumeRemoved(glib::Object<GVolume> const& volume);

  std::map<GVolume*, DeviceLauncherIcon::Ptr> map_;
  AbstractVolumeMonitorWrapper::Ptr monitor_;
  glib::Idle device_populate_idle_;
};

}
} // namespace unity

#endif // _DEVICE_LAUNCHER_SECTION_H_
