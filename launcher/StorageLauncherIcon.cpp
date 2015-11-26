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

#include "StorageLauncherIcon.h"

namespace unity
{
namespace launcher
{

StorageLauncherIcon::StorageLauncherIcon(AbstractLauncherIcon::IconType icon_type, FileManager::Ptr const& fm)
  : WindowedLauncherIcon(icon_type)
  , file_manager_(fm)
{
  file_manager_->locations_changed.connect(sigc::mem_fun(this, &StorageLauncherIcon::UpdateStorageWindows));
}

void StorageLauncherIcon::UpdateStorageWindows()
{
  bool active = false;
  bool urgent = false;
  managed_windows_ = GetStorageWindows();
  windows_connections_.Clear();

  for (auto const& win : managed_windows_)
  {
    windows_connections_.Add(win->monitor.changed.connect([this] (int) { EnsureWindowsLocation(); }));
    windows_connections_.Add(win->urgent.changed.connect([this] (bool) { OnWindowStateChanged(); }));
    windows_connections_.Add(win->active.changed.connect([this] (bool) { OnWindowStateChanged(); }));
    windows_connections_.Add(win->closed.connect([this] { UpdateStorageWindows(); }));

    if (!active && win->active())
      active = true;

    if (!urgent && win->urgent())
      urgent = true;
  }

  SetQuirk(Quirk::RUNNING, !managed_windows_.empty());
  SetQuirk(Quirk::ACTIVE, active);
  SetQuirk(Quirk::URGENT, urgent);

  EnsureWindowsLocation();
}

WindowList StorageLauncherIcon::GetManagedWindows() const
{
  return managed_windows_;
}

void StorageLauncherIcon::OnWindowStateChanged()
{
  bool active = false;
  bool urgent = false;

  for (auto const& win : managed_windows_)
  {
    if (!active && win->active())
      active = true;

    if (!urgent && win->urgent())
      urgent = true;
  }

  SetQuirk(Quirk::ACTIVE, active);
  SetQuirk(Quirk::URGENT, urgent);
}

} // namespace launcher
} // namespace unity
