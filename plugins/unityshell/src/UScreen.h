/*
 * Copyright (C) 2011 Canonical Ltd
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

#ifndef _UNITY_SCREEN_H_
#define _UNITY_SCREEN_H_

#include <gdk/gdk.h>
#include <Nux/Nux.h>
#include <sigc++/sigc++.h>
#include <vector>

#include "UnityCore/GLibDBusProxy.h"

class UScreen : public sigc::trackable
{
public:
  UScreen();
  ~UScreen();

  static UScreen* GetDefault();

  int             GetPrimaryMonitor();
  int             GetMonitorWithMouse();
  int             GetMonitorAtPosition(int x, int y);
  nux::Geometry&  GetMonitorGeometry(int monitor);

  std::vector<nux::Geometry>& GetMonitors();

  // <void, primary_monitor, monitors>
  sigc::signal<void, int, std::vector<nux::Geometry>&> changed;

  sigc::signal<void> resuming;

private:
  static void     Changed(GdkScreen* screen, UScreen* self);
  static gboolean OnIdleChanged(UScreen* self);
  void            Refresh();

private:
  std::vector<nux::Geometry> _monitors;
  guint32 _refresh_id;
  int primary_;
  unity::glib::DBusProxy proxy_;
};

#endif // _UNITY_SCREEN_H_
