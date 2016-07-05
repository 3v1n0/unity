// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2010, 2011, 2012 Canonical Ltd
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

#ifndef UNITYSHELL_SETTINGS_H
#define UNITYSHELL_SETTINGS_H

#include <memory>
#include <NuxCore/Property.h>
#include "EMConverter.h"

namespace unity
{

enum class FormFactor
{
  DESKTOP = 1,
  NETBOOK,
  TV
};

enum class LauncherPosition
{
  LEFT = 0,
  BOTTOM
};

enum class DesktopType
{
  UBUNTU,
  UBUNTUKYLIN
};

class Settings
{
public:
  Settings();
  ~Settings();

  static Settings& Instance();
  EMConverter::Ptr const& em(int monitor = 0) const;

  void SetLauncherSize(int launcher_size, int monitor);
  int LauncherSize(int mointor) const;

  nux::Property<bool> low_gfx;
  nux::RWProperty<FormFactor> form_factor;
  nux::Property<bool> is_standalone;
  nux::ROProperty<DesktopType> desktop_type;
  nux::ROProperty<bool> pam_check_account_type;
  nux::ROProperty<bool> double_click_activate;
  nux::Property<unsigned> lim_movement_thresold;
  nux::Property<unsigned> lim_double_click_wait;
  nux::Property<bool> lim_unfocused_popup;
  nux::Property<double> font_scaling;
  nux::ROProperty<bool> remote_content;
  nux::RWProperty<LauncherPosition> launcher_position;
  nux::Property<bool> gestures_launcher_drag;
  nux::Property<bool> gestures_dash_tap;
  nux::Property<bool> gestures_windows_drag_pinch;

  sigc::signal<void> dpi_changed;
  sigc::signal<void> low_gfx_changed;
  sigc::signal<void> gestures_changed;

private:
  class Impl;
  std::unique_ptr<Impl> pimpl;
};

}

#endif
