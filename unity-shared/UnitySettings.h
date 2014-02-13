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

class Settings
{
public:
  Settings();
  ~Settings();

  static Settings& Instance();
  bool GetLowGfxMode() const;
  void SetLowGfxMode(const bool low_gfx);

  nux::RWProperty<FormFactor> form_factor;
  nux::Property<bool> is_standalone;
  nux::ROProperty<bool> double_click_activate;
  nux::Property<unsigned> lim_movement_thresold;
  nux::Property<unsigned> lim_double_click_wait;

  EMConverter const& em(int monitor = 0) const;

private:
  class Impl;
  std::unique_ptr<Impl> pimpl;
};

}

#endif
