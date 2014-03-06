// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2013 Canonical Ltd
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
* Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
*/

#ifndef UNITY_LOCKSCREEN_SETTINGS_H
#define UNITY_LOCKSCREEN_SETTINGS_H

#include <NuxCore/Property.h>

namespace unity
{
namespace lockscreen
{

enum class Type : int
{
  NONE = 0, // Do nothing
  LIGHTDM, // Fallback to lightdm
  UNITY // Use custom Unity lockscreen
};

// TODO (andy) use the same options of unity-greeter

class Settings
{
public:
  Settings();
  ~Settings();

  static Settings& Instance();

  nux::Property<Type> lockscreen_type;
  nux::Property<std::string> font_name;
  nux::Property<std::string> logo;
  nux::Property<std::string> background;
  nux::Property<nux::Color> background_color;
  nux::Property<bool> show_hostname;
  nux::Property<bool> use_user_background;
  nux::Property<bool> draw_grid;

  static const int GRID_SIZE = 40;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}
}

#endif
