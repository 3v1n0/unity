// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2010 Canonical Ltd
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
* Authored by: Andrea Azzarone <aazzarone@hotmail.it>
*/

#include "DevicesSettings.h"

static DevicesSettings* _devices_settings = NULL;

DevicesSettings::DevicesSettings()
  : _settings(NULL),
    _raw_devices_option(1),
    _devices_option(ONLY_MOUNTED)
{
  _settings = g_settings_new("com.canonical.Unity.Devices");
  g_signal_connect(_settings, "changed",
                   (GCallback)(DevicesSettings::Changed), this);
  Refresh();
}

DevicesSettings::~DevicesSettings()
{
  g_object_unref(_settings);
}

void
DevicesSettings::Refresh()
{
  _raw_devices_option = g_settings_get_enum(_settings, "devices-option");

  _devices_option = (DevicesOption)_raw_devices_option;

  changed.emit(this);
}

void
DevicesSettings::Changed(GSettings* settings, char* key, DevicesSettings* self)
{
  self->Refresh();
}

DevicesSettings*
DevicesSettings::GetDefault()
{
  if (G_UNLIKELY(!_devices_settings))
    _devices_settings = new DevicesSettings();

  return _devices_settings;
}

DevicesSettings::DevicesOption
DevicesSettings::GetDevicesOption()
{
  return _devices_option;
}
