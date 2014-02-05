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

#include <UnityCore/GLibSignal.h>
#include <UnityCore/GLibSource.h>
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/GLibDBusProxy.h>

namespace unity
{

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
  nux::Geometry GetScreenGeometry();

  // <void, primary_monitor, monitors>
  sigc::signal<void, int, std::vector<nux::Geometry>&> changed;
  sigc::signal<void> resuming;

  const glib::Object<GdkScreen> &GetScreen() const;

private:
  void Changed(GdkScreen* screen);
  void Refresh();

protected:
  static UScreen* default_screen_;
  std::vector<nux::Geometry> monitors_;
  int primary_;

private:
  glib::Object<GdkScreen> screen_;
  glib::DBusProxy proxy_;
  glib::Signal<void, GdkScreen*> size_changed_signal_;
  glib::Signal<void, GdkScreen*> monitors_changed_signal_;
  glib::Source::UniquePtr refresh_idle_;
};

} // Namespace
#endif // _UNITY_SCREEN_H_
