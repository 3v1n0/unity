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

#include "SimpleLauncherIcon.h"
#include "DevicesSettings.h"

#include <gio/gio.h>

class DeviceLauncherIcon : public SimpleLauncherIcon
{

public:
  DeviceLauncherIcon  (Launcher *launcher, GVolume *volume);
  ~DeviceLauncherIcon ();
  
  virtual nux::Color BackgroundColor ();
  virtual nux::Color GlowColor ();
  void UpdateVisibility ();
  bool CanEject ();
  void Eject ();

protected:
  std::list<DbusmenuMenuitem *> GetMenus ();
  void UpdateDeviceIcon ();

private:
  void ActivateLauncherIcon (ActionArg arg);
  void ShowMount (GMount *mount);
  void Unmount ();
  void StopDrive ();
  static void OnOpen (DbusmenuMenuitem *item, int time, DeviceLauncherIcon *self);
  static void OnEject (DbusmenuMenuitem *item, int time, DeviceLauncherIcon *self);
  static void OnUnmount (DbusmenuMenuitem *item, int time, DeviceLauncherIcon *self);
  static void OnRemoved (GVolume *volume, DeviceLauncherIcon *self);
  static void OnChanged (GVolume *volume, DeviceLauncherIcon *self);
  static void OnMountReady (GObject *object, GAsyncResult *result, DeviceLauncherIcon *self);
  static void OnEjectReady (GObject *object, GAsyncResult *result, DeviceLauncherIcon *self);
  static void OnUnmountReady (GObject *object, GAsyncResult *result, DeviceLauncherIcon *self);
  static void OnDriveStop (DbusmenuMenuitem *item, int time, DeviceLauncherIcon *self);
  static void OnStopDriveReady (GObject *object, GAsyncResult *result, DeviceLauncherIcon *self);
  void OnSettingsChanged (DevicesSettings *settings);

private:
  GVolume *_volume;
  gulong _on_removed_handler_id;
  gulong _on_changed_handler_id;
};

#endif // _DEVICE_LAUNCHER_ICON_H__H
