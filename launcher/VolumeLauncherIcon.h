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

#include "Volume.h"
#include "DevicesSettings.h"
#include "DeviceNotificationDisplay.h"
#include "StorageLauncherIcon.h"
#include "unity-shared/FileManager.h"

namespace unity
{
namespace launcher
{

class VolumeLauncherIcon : public StorageLauncherIcon
{
public:
  typedef nux::ObjectPtr<VolumeLauncherIcon> Ptr;

  VolumeLauncherIcon(Volume::Ptr const&, DevicesSettings::Ptr const&,
                     DeviceNotificationDisplay::Ptr const&, FileManager::Ptr const&);
  virtual ~VolumeLauncherIcon();

  void AboutToRemove() override;

  bool CanEject() const; // TODO: rename to public virtual bool IsTrashable();
  void EjectAndShowNotification(); // TODO: rename to private virtual void DoDropToTrash();
  bool CanStop() const;
  void StopDrive();
  void Stick(bool save = true);
  void UnStick();

  MenuItemsVector GetMenus();
  std::string GetRemoteUri() const;
  std::string GetVolumeUri() const;

protected:
  void OnAcceptDrop(DndData const&);
  nux::DndAction OnQueryAcceptDrop(DndData const&);
  WindowList GetStorageWindows() const override;
  void OpenInstanceLauncherIcon(Time timestamp) override;

  // Introspection
  virtual std::string GetName() const;

private:
  class Impl;
  std::unique_ptr<Impl> pimpl_;
};

}
}

#endif
