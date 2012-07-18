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

#ifndef UNITYSHELL_VOLUME_MONITOR_WRAPPER_H
#define UNITYSHELL_VOLUME_MONITOR_WRAPPER_H

#include <list>
#include <memory>

#include <gio/gio.h>
#include <sigc++/signal.h>
#include <sigc++/trackable.h>

#include <UnityCore/GLibWrapper.h>
#include <UnityCore/GLibSignal.h>

namespace unity
{
namespace launcher
{

class VolumeMonitorWrapper : public sigc::trackable
{
public:
  typedef std::shared_ptr<VolumeMonitorWrapper> Ptr;
  typedef std::list<glib::Object<GVolume>> VolumeList;

  VolumeMonitorWrapper();

  // Makes VolumeMonitorWrapper uncopyable
  VolumeMonitorWrapper(VolumeMonitorWrapper const&) = delete;
  VolumeMonitorWrapper& operator=(VolumeMonitorWrapper const&) = delete;

  VolumeList GetVolumes();

  // Signals
  sigc::signal<void, glib::Object<GVolume> const&> volume_added;
  sigc::signal<void, glib::Object<GVolume> const&> volume_removed;

private:
  void OnVolumeAdded(GVolumeMonitor* monitor, GVolume* volume);
  void OnVolumeRemoved(GVolumeMonitor* monitor, GVolume* volume);

  glib::Object<GVolumeMonitor> monitor_;
  glib::SignalManager sig_manager_;
};

} // namespace launcher
} // namespace unity

#endif // UNITYSHELL_VOLUME_MONITOR_WRAPPER_H

