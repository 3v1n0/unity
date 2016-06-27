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
#include <vector>
#include <iostream>
#include <sigc++/sigc++.h>
#include <NuxCore/Rect.h>

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

  int GetPrimaryMonitor() const;
  int GetMonitorWithMouse() const;
  int GetMonitorAtPosition(int x, int y) const;
  nux::Geometry const& GetMonitorGeometry(int monitor) const;
  nux::Size const& GetMonitorPhysicalSize(int monitor) const;

  std::vector<nux::Geometry> const& GetMonitors() const;
  nux::Geometry GetScreenGeometry() const;

  // <void, primary_monitor, monitors>
  sigc::signal<void, int, std::vector<nux::Geometry> const&> changed;
  sigc::signal<void> resuming;

  const std::string GetMonitorName(int output_number) const;
  int GetPluggedMonitorsNumber() const;

private:
  void Changed(GdkScreen* screen);
  void Refresh();

protected:
  static UScreen* default_screen_;
  std::vector<nux::Geometry> monitors_;
  std::vector<nux::Size> physical_monitors_;
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
