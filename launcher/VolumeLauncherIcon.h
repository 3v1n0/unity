// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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

#ifndef UNITYSHELL_VOLUME_LAUNCHER_ICON_H
#define UNITYSHELL_VOLUME_LAUNCHER_ICON_H

#include <gio/gio.h>
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/GLibSignal.h>

#include "SimpleLauncherIcon.h"

namespace unity
{
namespace launcher
{

class DevicesSettings;

class VolumeLauncherIcon : public SimpleLauncherIcon
{
public:
  typedef nux::ObjectPtr<VolumeLauncherIcon> Ptr;

  VolumeLauncherIcon(glib::Object<GVolume> const& volume,
                     std::shared_ptr<DevicesSettings> const& devices_settings);

  void OnRemoved();
  bool CanEject();
  void Eject();

protected:
  std::list<DbusmenuMenuitem*> GetMenus();
  std::string GetName() const;

private:
  void UpdateVisibility();
  void UpdateIcon();
  void ActivateLauncherIcon(ActionArg arg);
  void ShowMount(GMount* mount);
  void Unmount();
  void StopDrive();
  static void OnTogglePin(DbusmenuMenuitem* item, int time, VolumeLauncherIcon* self);
  static void OnOpen(DbusmenuMenuitem* item, int time, VolumeLauncherIcon* self);
  static void OnEject(DbusmenuMenuitem* item, int time, VolumeLauncherIcon* self);
  static void OnUnmount(DbusmenuMenuitem* item, int time, VolumeLauncherIcon* self);
  static void OnChanged(GVolume* volume, VolumeLauncherIcon* self);
  static void OnMountReady(GObject* object, GAsyncResult* result, VolumeLauncherIcon* self);
  static void OnEjectReady(GObject* object, GAsyncResult* result, VolumeLauncherIcon* self);
  static void OnUnmountReady(GObject* object, GAsyncResult* result, VolumeLauncherIcon* self);
  static void OnDriveStop(DbusmenuMenuitem* item, int time, VolumeLauncherIcon* self);
  void OnVolumeChanged(GVolume* volume);
  void OnSettingsChanged();
  void ShowNotification(std::string const&, unsigned, glib::Object<GdkPixbuf> const&, std::string const&);

private:
  glib::Object<GVolume> volume_;
  std::shared_ptr<DevicesSettings> devices_settings_;

  std::string name_;
  bool keep_in_launcher_;

  glib::Signal<void, GVolume*> signal_volume_changed_;
  glib::Source::UniquePtr changed_timeout_;
};

}
} // namespace unity

#endif // _DEVICE_LAUNCHER_ICON_H__H
