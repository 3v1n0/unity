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

#include "AppmenuIndicator.h"

namespace unity
{
namespace indicator
{

const std::string SETTING_NAME("com.canonical.indicator.appmenu");
const std::string SETTING_KEY("integrated");

AppmenuIndicator::AppmenuIndicator(std::string const& name)
  : Indicator(name),
    gsettings_(g_settings_new(SETTING_NAME.c_str())),
    integrated_(false)
{
  setting_changed_.Connect(gsettings_, "changed", [&] (GSettings*, gchar* key) {
    if (g_strcmp0(key, SETTING_KEY.c_str()) == 0)
      CheckSettingValue();
  });

  CheckSettingValue();
}

void AppmenuIndicator::ShowAppmenu() const
{
  g_debug("Showing menu!");
}

bool AppmenuIndicator::IsIntegrated() const
{
  return integrated_;
}

void AppmenuIndicator::CheckSettingValue()
{
  glib::String menu_mode(g_settings_get_string(gsettings_, "menu-mode"));
  bool integrated_menus = false;

  if (menu_mode.Str() == SETTING_KEY)
  {
    integrated_menus = true;
  }

  if (integrated_menus != integrated_)
  {
    integrated_ = integrated_menus;
    integrated_changed(integrated_);
  }
}

} // namespace indicator
} // namespace unity
