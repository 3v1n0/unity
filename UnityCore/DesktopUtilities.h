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

#ifndef UNITY_DESKTOP_UTILITIES_H
#define UNITY_DESKTOP_UTILITIES_H

#include <string>
#include <vector>

namespace unity
{

class DesktopUtilities
{
public:
  static std::string GetUserDataDirectory();
  static std::vector<std::string> GetSystemDataDirectories();
  static std::vector<std::string> GetDataDirectories();

  static std::string GetDesktopID(std::string const& desktop_path);
  static std::string GetDesktopID(std::vector<std::string> const& default_paths,
                                  std::string const& desktop_path);
  static std::string GetDesktopPathById(std::string const& desktop_id);

  static std::string GetBackgroundColor(std::string const& desktop_path);
};

} // namespace

#endif
