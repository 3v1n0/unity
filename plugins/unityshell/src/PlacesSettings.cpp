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

#include "PlacesSettings.h"
#include "PlacesStyle.h"

#define HOME_EXPANDED "home-expanded"

static PlacesSettings* _places_settings = NULL;

PlacesSettings::PlacesSettings()
  : _settings(NULL),
    _raw_from_factor(0),
    _form_factor(DESKTOP),
    _dash_blur_type(NO_BLUR)
{
  _settings = g_settings_new("com.canonical.Unity");
  g_signal_connect(_settings, "changed",
                   (GCallback)(PlacesSettings::Changed), this);
  Refresh();
}

PlacesSettings::~PlacesSettings()
{
  g_object_unref(_settings);
}

void
PlacesSettings::Refresh()
{
  _raw_from_factor = g_settings_get_enum(_settings, "form-factor");

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

  changed.emit(this);
}

void
PlacesSettings::Changed(GSettings* settings, char* key, PlacesSettings* self)
{
  self->Refresh();
}

PlacesSettings*
PlacesSettings::GetDefault()
{
  if (G_UNLIKELY(!_places_settings))
    _places_settings = new PlacesSettings();

  return _places_settings;
}

PlacesSettings::FormFactor
PlacesSettings::GetFormFactor()
{
  return _form_factor;
}

int
PlacesSettings::GetDefaultTileWidth()
{
  return PlacesStyle::GetDefault()->GetTileWidth();
}

PlacesSettings::DashBlurType
PlacesSettings::GetDashBlurType()
{
  return _dash_blur_type;
}

void
PlacesSettings::SetDashBlurType(PlacesSettings::DashBlurType type)
{
  _dash_blur_type = type;
}

bool
PlacesSettings::GetHomeExpanded()
{
  return g_settings_get_enum(_settings, HOME_EXPANDED) == 1 ? true : false;
}

void
PlacesSettings::SetHomeExpanded(bool expanded)
{
  g_settings_set_enum(_settings, HOME_EXPANDED, expanded ? 1 : 0);
}
