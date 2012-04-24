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
 */

#ifndef _DEVICE_LAUNCHER_ICON_H__H
#define _DEVICE_LAUNCHER_ICON_H__H

#include <gio/gio.h>
#define GDU_API_IS_SUBJECT_TO_CHANGE
G_BEGIN_DECLS
#include <gdu/gdu.h>
#include <UnityCore/GLibWrapper.h>
#include "SimpleLauncherIcon.h"

namespace unity
{
namespace launcher
{

class DeviceLauncherIcon : public SimpleLauncherIcon
{

public:
  DeviceLauncherIcon(glib::Object<GVolume> const& volume);

  void UpdateVisibility(int visibility = -1);
  void OnRemoved();
  bool CanEject();
  void Eject();

protected:
  std::list<DbusmenuMenuitem*> GetMenus();
  void UpdateDeviceIcon();
  std::string GetName() const;

private:
  void ActivateLauncherIcon(ActionArg arg);
  void ShowMount(GMount* mount);
  void Unmount();
  void StopDrive();
  static void OnTogglePin(DbusmenuMenuitem* item, int time, DeviceLauncherIcon* self);
  static void OnOpen(DbusmenuMenuitem* item, int time, DeviceLauncherIcon* self);
  static void OnFormat(DbusmenuMenuitem* item, int time, DeviceLauncherIcon* self);
  static void OnEject(DbusmenuMenuitem* item, int time, DeviceLauncherIcon* self);
  static void OnUnmount(DbusmenuMenuitem* item, int time, DeviceLauncherIcon* self);
  static void OnChanged(GVolume* volume, DeviceLauncherIcon* self);
  static void OnMountReady(GObject* object, GAsyncResult* result, DeviceLauncherIcon* self);
  static void OnEjectReady(GObject* object, GAsyncResult* result, DeviceLauncherIcon* self);
  static void OnUnmountReady(GObject* object, GAsyncResult* result, DeviceLauncherIcon* self);
  static void OnDriveStop(DbusmenuMenuitem* item, int time, DeviceLauncherIcon* self);
  void OnSettingsChanged();
  void ShowNotification(std::string const&, unsigned, GdkPixbuf*, std::string const&);

private:
  glib::Object<GVolume> volume_;
  glib::String device_file_;
  std::string name_;
  glib::Object<GduDevice> gdu_device_;
  bool keep_in_launcher_;
};

}
} // namespace unity

#endif // _DEVICE_LAUNCHER_ICON_H__H
