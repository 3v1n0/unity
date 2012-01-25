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

#ifndef UNITY_APPMENU_INDICATOR_H
#define UNITY_APPMENU_INDICATOR_H

#include "Indicator.h"
#include "GLibWrapper.h"
#include "GLibSignal.h"

#include <gio/gio.h>

namespace unity
{
namespace indicator
{

class AppmenuIndicator : public Indicator
{
public:
  AppmenuIndicator(std::string const& name);

  virtual bool IsAppmenu() const { return true; }
  bool IsIntegrated() const;

  void ShowAppmenu(unsigned int xid, int x, int y, unsigned int timestamp) const;

  sigc::signal<void, bool> integrated_changed;
  sigc::signal<void, unsigned int, int, int, unsigned int> show_appmenu;

private:
  void CheckSettingValue();

  glib::Object<GSettings> gsettings_;
  glib::Signal<void, GSettings*, gchar*> setting_changed_;
  bool integrated_;
};


}
}

#endif // UNITY_APPMENU_INDICATOR_H
