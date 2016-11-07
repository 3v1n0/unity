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
#include "DeviceLauncherSection.h"

namespace unity
{
namespace launcher
{

class FileManagerLauncherIcon : public ApplicationLauncherIcon, public StorageLauncherIcon
{
public:
  typedef nux::ObjectPtr<FileManagerLauncherIcon> Ptr;

  FileManagerLauncherIcon(ApplicationPtr const&, DeviceLauncherSection::Ptr const&, FileManager::Ptr const& = nullptr);

  bool IsUserVisible() const override;

protected:
  WindowList WindowsOnViewport() override;
  WindowList WindowsForMonitor(int monitor) override;

private:
  WindowList GetManagedWindows() const override;
  WindowList GetStorageWindows() const override;
  void Focus(ActionArg arg) override;
  void Quit() const override;
  bool OnShouldHighlightOnDrag(DndData const& dnd_data) override;

  bool IsLocationManaged(std::string const&) const;

  DeviceLauncherSection::Ptr devices_;
};

} // namespace launcher
} // namespace unity

#endif // FILEMANAGER_LAUNCHER_ICON_H
