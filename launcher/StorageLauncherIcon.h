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

#ifndef STORAGE_LAUNCHER_ICON_H
#define STORAGE_LAUNCHER_ICON_H

#include "WindowedLauncherIcon.h"
#include "unity-shared/FileManager.h"

namespace unity
{
namespace launcher
{

class StorageLauncherIcon : public virtual WindowedLauncherIcon
{
public:
  StorageLauncherIcon(AbstractLauncherIcon::IconType, FileManager::Ptr const&);

protected:
  void UpdateStorageWindows();
  WindowList GetManagedWindows() const override;
  virtual WindowList GetStorageWindows() const = 0;

  bool OnShouldHighlightOnDrag(DndData const& dnd_data) override;

private:
  void OnWindowStateChanged();

protected:
  FileManager::Ptr file_manager_;
  WindowList managed_windows_;
  connection::Manager windows_connections_;
};

} // namespace launcher
} // namespace unity

#endif // STORAGE_LAUNCHER_ICON_H
