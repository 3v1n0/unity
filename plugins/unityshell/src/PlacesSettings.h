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

#ifndef PLACES_SETTINGS_H
#define PLACES_SETTINGS_H

#include <gio/gio.h>
#include <Nux/Nux.h>

#include "PlacesSettings.h"

class PlacesSettings : public nux::Object
{
public:
  enum FormFactor
  {
    DESKTOP = 1,
    NETBOOK

  };

  PlacesSettings();
  ~PlacesSettings();

  static PlacesSettings* GetDefault();

  FormFactor GetFormFactor();
  int        GetDefaultTileWidth();

  bool GetHomeExpanded();
  void SetHomeExpanded(bool expanded);

  sigc::signal<void, PlacesSettings*> changed;

private:
  void Refresh();
  static void Changed(GSettings* settings, gchar* key, PlacesSettings* self);

private:
  GSettings*   _settings;
  int          _raw_from_factor;
  FormFactor   _form_factor;
};

#endif // PLACES_SETTINGS_H
