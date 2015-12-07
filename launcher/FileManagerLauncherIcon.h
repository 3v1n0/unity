// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2015 Canonical Ltd
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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#ifndef FILEMANAGER_LAUNCHER_ICON_H
#define FILEMANAGER_LAUNCHER_ICON_H

#include "ApplicationLauncherIcon.h"
#include "StorageLauncherIcon.h"

namespace unity
{
namespace launcher
{

class FileManagerLauncherIcon : public ApplicationLauncherIcon, public StorageLauncherIcon
{
public:
  FileManagerLauncherIcon(ApplicationPtr const&, FileManager::Ptr const& = nullptr);

private:
  // IconType GetIconType() const override {return ApplicationLauncherIcon::GetIconType(); }
  WindowList GetManagedWindows() const override;
  WindowList GetStorageWindows() const override;

  void SetQuirk(Quirk quirk, bool value, int monitor) override { ApplicationLauncherIcon::SetQuirk(quirk, value, monitor); }
  bool GetQuirk(Quirk quirk, int monitor) const override { return ApplicationLauncherIcon::GetQuirk(quirk, monitor); }

  void Focus(ActionArg arg) override { StorageLauncherIcon::Focus(arg); }
  WindowList Windows() override { return StorageLauncherIcon::Windows(); }
  WindowList WindowsForMonitor(int monitor) override { return StorageLauncherIcon::WindowsForMonitor(monitor); }
  WindowList WindowsOnViewport() override { return StorageLauncherIcon::WindowsOnViewport(); }
  bool WindowVisibleOnMonitor(int monitor) const override { return StorageLauncherIcon::WindowVisibleOnMonitor(monitor); }
  bool WindowVisibleOnViewport() const override { return StorageLauncherIcon::WindowVisibleOnViewport(); }
  size_t WindowsVisibleOnMonitor(int monitor) const override { return StorageLauncherIcon::WindowsVisibleOnMonitor(monitor); }
  size_t WindowsVisibleOnViewport() const override { return StorageLauncherIcon::WindowsVisibleOnViewport(); }

  bool IsLocationManaged(std::string const&) const;
};

} // namespace launcher
} // namespace unity

#endif // FILEMANAGER_LAUNCHER_ICON_H
