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

#include "FileManagerLauncherIcon.h"

#include <boost/algorithm/string.hpp>
#include <NuxCore/Logger.h>
#include <UnityCore/DesktopUtilities.h>

#include "unity-shared/GnomeFileManager.h"

namespace unity
{
namespace launcher
{
namespace
{
DECLARE_LOGGER(logger, "unity.launcher.icon.filemanager");
const std::string TRASH_URI = "trash:";
const std::string TRASH_PATH = "file://" + DesktopUtilities::GetUserTrashDirectory();
const std::string DEFAULT_ICON = "system-file-manager";
}

FileManagerLauncherIcon::FileManagerLauncherIcon(ApplicationPtr const& app, DeviceLauncherSection::Ptr const& dev, FileManager::Ptr const& fm)
  : WindowedLauncherIcon(IconType::APPLICATION)
  , ApplicationLauncherIcon(app)
  , StorageLauncherIcon(GetIconType(), fm ? fm : GnomeFileManager::Get())
  , devices_(dev)
{
  // We disconnect from ApplicationLauncherIcon app signals, as we manage them manually
  signals_conn_.Clear();

  signals_conn_.Add(app_->desktop_file.changed.connect([this](std::string const& desktop_file) {
    LOG_DEBUG(logger) << tooltip_text() << " desktop_file now " << desktop_file;
    UpdateDesktopFile();
  }));

  signals_conn_.Add(app_->closed.connect([this] {
    LOG_DEBUG(logger) << tooltip_text() << " closed";
    OnApplicationClosed();
  }));

  signals_conn_.Add(app_->title.changed.connect([this](std::string const& name) {
    LOG_DEBUG(logger) << tooltip_text() << " name now " << name;
    menu_items_.clear();
    tooltip_text = name;
  }));

  signals_conn_.Add(app_->icon.changed.connect([this](std::string const& icon) {
    LOG_DEBUG(logger) << tooltip_text() << " icon now " << icon;
    icon_name = (icon.empty() ? DEFAULT_ICON : icon);
  }));

  signals_conn_.Add(app_->running.changed.connect([this](bool running) {
    LOG_DEBUG(logger) << tooltip_text() << " running now " << (running ? "true" : "false");

    if (running)
      _source_manager.Remove(ICON_REMOVE_TIMEOUT);
  }));


  UpdateStorageWindows();
}

void FileManagerLauncherIcon::Focus(ActionArg arg)
{
  WindowedLauncherIcon::Focus(arg);
}

void FileManagerLauncherIcon::Quit() const
{
  WindowedLauncherIcon::Quit();
}

bool FileManagerLauncherIcon::IsLocationManaged(std::string const& location) const
{
  if (location.empty())
    return true;

  if (boost::algorithm::starts_with(location, TRASH_URI))
    return false;

  if (boost::algorithm::starts_with(location, TRASH_PATH))
    return false;

  for (auto const& volume_icon : devices_->GetIcons())
  {
    auto const& volume_uri = volume_icon->GetVolumeUri();
    if (!volume_uri.empty() && boost::algorithm::starts_with(location, volume_uri))
      return false;
  }

  return true;
}

WindowList FileManagerLauncherIcon::GetManagedWindows() const
{
  return StorageLauncherIcon::GetManagedWindows();
}

WindowList FileManagerLauncherIcon::GetStorageWindows() const
{
  WindowList fm_windows;

  for (auto const& app_win : ApplicationLauncherIcon::GetManagedWindows())
  {
    if (IsLocationManaged(file_manager_->LocationForWindow(app_win)))
      fm_windows.push_back(app_win);
  }

  return fm_windows;
}

bool FileManagerLauncherIcon::OnShouldHighlightOnDrag(DndData const& dnd_data)
{
  return StorageLauncherIcon::OnShouldHighlightOnDrag(dnd_data);
}

} // namespace launcher
} // namespace unity
