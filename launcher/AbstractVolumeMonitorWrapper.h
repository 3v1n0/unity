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

#ifndef UNITYSHELL_ABSTRACT_VOLUME_MONITOR_WRAPPER_H
#define UNITYSHELL_ABSTRACT_VOLUME_MONITOR_WRAPPER_H

#include <list>
#include <memory>

#include <gio/gio.h>
#include <sigc++/signal.h>
#include <sigc++/trackable.h>

#include <UnityCore/GLibWrapper.h>

namespace unity
{
namespace launcher
{

class AbstractVolumeMonitorWrapper : public sigc::trackable
{
public:
  typedef std::shared_ptr<AbstractVolumeMonitorWrapper> Ptr;
  typedef std::list<glib::Object<GVolume>> VolumeList;

  AbstractVolumeMonitorWrapper() = default;
  virtual ~AbstractVolumeMonitorWrapper() {};

  // Makes VolumeMonitorWrapper uncopyable
  AbstractVolumeMonitorWrapper(AbstractVolumeMonitorWrapper const&) = delete;
  AbstractVolumeMonitorWrapper& operator=(AbstractVolumeMonitorWrapper const&) = delete;

  virtual VolumeList GetVolumes() = 0;

  // Signals
  sigc::signal<void, glib::Object<GVolume> const&> volume_added;
  sigc::signal<void, glib::Object<GVolume> const&> volume_removed;
};

}
}

#endif

