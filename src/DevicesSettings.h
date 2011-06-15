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

#ifndef DEVICES_SETTINGS_H
#define DEVICES_SETTINGS_H

#include <gio/gio.h>
#include <Nux/Nux.h>


class DevicesSettings : public nux::Object
{
public:
  typedef enum
  {
    NEVER=0,
    ONLY_MOUNTED,
    ALWAYS

  } DevicesOption;

  DevicesSettings ();
  ~DevicesSettings ();

  static DevicesSettings * GetDefault ();

  void          SetDevicesOption (DevicesOption devices_option);
  DevicesOption GetDevicesOption () { return _devices_option; };

  GSList * GetFavorites ();
  void     AddFavorite    (const char *uuid);
  void     RemoveFavorite (const char *uuid);

  sigc::signal<void, DevicesSettings *> changed;

private:
  void Refresh ();
  static void Changed (GSettings *settings, gchar *key, DevicesSettings *self);

private:
  GSettings    *_settings;
  GSList       *_favorites;
  bool          _ignore_signals;
  DevicesOption _devices_option;
};

#endif // DEVICES_SETTINGS_H
