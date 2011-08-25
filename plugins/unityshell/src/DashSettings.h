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

#ifndef DASH_SETTINGS_H
#define DASH_SETTINGS_H

#include <gio/gio.h>
#include <Nux/Nux.h>

#include "DashSettings.h"

class DashSettings : public nux::Object
{
public:
  enum FormFactor
  {
    DESKTOP = 1,
    NETBOOK
  };

  DashSettings();
  ~DashSettings();

  static DashSettings* GetDefault();

  FormFactor GetFormFactor();
  void SetFormFactor(FormFactor factor);

  sigc::signal<void> changed;

private:
  void Refresh();
  static void Changed(GSettings* settings, gchar* key, DashSettings* self);

private:
  GSettings*   _settings;
  int          _raw_from_factor;
  FormFactor   _form_factor;
};

#endif // DASH_SETTINGS_H
