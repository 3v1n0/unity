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
* Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
*/

#include "gdk/gdk.h"

#include "DashSettings.h"

#define FORM_FACTOR "form-factor"

static DashSettings* _places_settings = NULL;

DashSettings::DashSettings()
  : _settings(NULL),
    _raw_from_factor(0),
    _form_factor(DESKTOP)
{
  _settings = g_settings_new("com.canonical.Unity");
  g_signal_connect(_settings, "changed",
                   (GCallback)(DashSettings::Changed), this);
  Refresh();
}

DashSettings::~DashSettings()
{
  g_object_unref(_settings);
}

void
DashSettings::Refresh()
{
  _raw_from_factor = g_settings_get_enum(_settings, FORM_FACTOR);

  if (_raw_from_factor == 0) //Automatic
  {
    GdkScreen*   screen;
    gint         primary_monitor;
    GdkRectangle geo;

    screen = gdk_screen_get_default();
    primary_monitor = gdk_screen_get_primary_monitor(screen);
    gdk_screen_get_monitor_geometry(screen, primary_monitor, &geo);

    _form_factor = geo.height > 799 ? DESKTOP : NETBOOK;
  }
  else
  {
    _form_factor = (FormFactor)_raw_from_factor;
  }

  changed.emit();
}

void
DashSettings::Changed(GSettings* settings, char* key, DashSettings* self)
{
  self->Refresh();
}

DashSettings*
DashSettings::GetDefault()
{
  if (G_UNLIKELY(!_places_settings))
    _places_settings = new DashSettings();

  return _places_settings;
}

DashSettings::FormFactor
DashSettings::GetFormFactor()
{
  return _form_factor;
}

void
DashSettings::SetFormFactor(FormFactor factor)
{
  _form_factor = factor;
  g_settings_set_enum(_settings, FORM_FACTOR, factor);
}
