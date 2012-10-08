// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010-2012 Canonical Ltd
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
 * Authored by: Jason Smith <jason.smith@canonical.com>
 *              Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 */

#ifndef BAMFLAUNCHERICON_H
#define BAMFLAUNCHERICON_H

#include <UnityCore/GLibSignal.h>
#include <UnityCore/GLibWrapper.h>

#include <libbamf/libbamf.h>
#include <libindicator/indicator-desktop-shortcuts.h>

#include "SimpleLauncherIcon.h"

namespace unity
{
namespace launcher
{

class Launcher;

class ApplicationLauncherIcon : public SimpleLauncherIcon
{
  NUX_DECLARE_OBJECT_TYPE(ApplicationLauncherIcon, SimpleLauncherIcon);
public:
  ApplicationLauncherIcon(BamfApplication* app);
  virtual ~ApplicationLauncherIcon();

  virtual void ActivateLauncherIcon(ActionArg arg);

  std::string DesktopFile();

  bool IsSticky() const;
  bool IsActive() const;
  bool IsRunning() const;
  bool IsUrgent() const;

  virtual void Quit();
  virtual void Stick(bool save = true);
  virtual void UnStick();

  virtual bool ShowInSwitcher(bool current);
  virtual unsigned long long SwitcherPriority();

  virtual nux::Color BackgroundColor() const;

  std::vector<Window> Windows();
  std::vector<Window> WindowsOnViewport();
  std::vector<Window> WindowsForMonitor(int monitor);
  std::string NameForWindow(Window window);

protected:
  void Remove();
  void UpdateIconGeometries(std::vector<nux::Point3> center);
  void OnCenterStabilized(std::vector<nux::Point3> center);
  void AddProperties(GVariantBuilder* builder);
  void OnAcceptDrop(DndData const& dnd_data);
  void OnDndEnter();
  void OnDndHovered();
  void OnDndLeave();
  void OpenInstanceLauncherIcon(ActionArg arg);
  void ToggleSticky();

  bool OnShouldHighlightOnDrag(DndData const& dnd_data);
  nux::DndAction OnQueryAcceptDrop(DndData const& dnd_data);

  MenuItemsVector GetMenus();
  std::set<std::string> ValidateUrisForLaunch(DndData const& dnd_data);

  std::string GetRemoteUri();
  std::string BamfName() const;

  bool HandlesSpread() { return true; }
  std::string GetName() const;

private:
  typedef unsigned long int WindowFilterMask;
  enum WindowFilter
  {
    MAPPED = (1 << 0),
    USER_VISIBLE = (1 << 1),
    ON_CURRENT_DESKTOP = (1 << 2),
    ON_ALL_MONITORS = (1 << 3),
  };

  void EnsureWindowState();
  void EnsureMenuItemsReady();
  void UpdateBackgroundColor();
  void UpdateDesktopFile();
  void UpdateMenus();
  void UpdateDesktopQuickList();

  void OpenInstanceWithUris(std::set<std::string> uris);
  void Focus(ActionArg arg);
  std::vector<Window> GetFocusableWindows(ActionArg arg, bool &any_visible, bool &any_urgent);
  bool Spread(bool current_desktop, int state, bool force);

  void OnWindowMinimized(guint32 xid);
  void OnWindowMoved(guint32 xid);

  bool OwnsWindow(Window w) const;

  std::vector<Window> GetWindows(WindowFilterMask filter = 0, int monitor = -1);
  const std::set<std::string> GetSupportedTypes();
  std::string GetDesktopID();

  glib::Object<BamfApplication> _bamf_app;

  std::string _remote_uri;
  std::string _desktop_file;
  std::map<std::string, glib::Object<DbusmenuClient>> _menu_clients;
  std::map<std::string, glib::Object<DbusmenuMenuitem>> _menu_items;
  std::map<std::string, glib::Object<DbusmenuMenuitem>> _menu_items_extra;
  glib::Object<IndicatorDesktopShortcuts> _desktop_shortcuts;
  glib::Object<DbusmenuMenuitem> _menu_desktop_shortcuts;
  glib::Object<GFileMonitor> _desktop_file_monitor;
  glib::SignalManager _gsignals;

  bool use_custom_bg_color_;
  nux::Color bg_color_;
};

}
}

#endif // BAMFLAUNCHERICON_H
