// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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
* Authored by: Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
*/

#include <glib.h>
#include <algorithm>

#include "DesktopUtilities.h"

namespace unity
{

std::string DesktopUtilities::GetUserDataDirectory()
{
  const char* user_dir = g_get_user_data_dir();

  if (user_dir)
    return user_dir;

  // This shouldn't ever happen, but let's manually fallback to the default
  const char* home = g_get_home_dir();

  if (home)
  {
    const char* subdir = G_DIR_SEPARATOR_S ".local" G_DIR_SEPARATOR_S "share";
    return std::string(home).append(subdir);
  }

  return "";
}

std::vector<std::string> DesktopUtilities::GetSystemDataDirectories()
{
  const char* const* system_dirs = g_get_system_data_dirs();
  std::vector<std::string> directories;

  for (unsigned int i = 0; system_dirs && system_dirs[i]; ++i)
  {
    if (system_dirs[i][0] != '\0')
    {
      std::string dir(system_dirs[i]);
      directories.push_back(dir);
    }
  }

  return directories;
}

std::vector<std::string> DesktopUtilities::GetDataDirectories()
{
  std::vector<std::string> dirs = GetSystemDataDirectories();
  std::string const& user_directory = GetUserDataDirectory();  

  dirs.push_back(user_directory);

  return dirs;
}

std::string DesktopUtilities::GetDesktopID(std::vector<std::string> const& default_paths,
                                           std::string const& desktop_path)
{
  /* We check to see if the desktop file belongs to one of the system data
   * directories. If so, then we store it's desktop id, otherwise we store
   * it's full path. We're clever like that.
   */

  if (desktop_path.empty())
    return "";

  for (auto dir : default_paths)
  {
    if (!dir.empty())
    {
      if (dir.at(dir.length()-1) == G_DIR_SEPARATOR)
      {
        dir.append("applications" G_DIR_SEPARATOR_S);
      }
      else
      {
        dir.append(G_DIR_SEPARATOR_S "applications" G_DIR_SEPARATOR_S);
      }

      if (desktop_path.find(dir) == 0)
      {
        // if we are in a subdirectory of system path, the store name should
        // be subdir-filename.desktop
        std::string desktop_suffix = desktop_path.substr(dir.size());
        std::replace(desktop_suffix.begin(), desktop_suffix.end(), G_DIR_SEPARATOR, '-');

        return desktop_suffix;
      }
    }
  }

  return desktop_path;
}

std::string DesktopUtilities::GetDesktopID(std::string const& desktop_path)
{
  std::vector<std::string> const& data_dirs = GetDataDirectories();
  return GetDesktopID(data_dirs, desktop_path);
}

} // namespace unity
