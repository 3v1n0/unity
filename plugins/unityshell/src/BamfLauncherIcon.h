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

#include <Nux/BaseWindow.h>
#include <NuxCore/Math/MathInc.h>

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

class BamfLauncherIcon : public SimpleLauncherIcon
{
public:
  BamfLauncherIcon(Launcher* IconManager, BamfApplication* app);
  virtual ~BamfLauncherIcon();

  void ActivateLauncherIcon(ActionArg arg);

  const char* DesktopFile();

  bool IsSticky() const;
  bool IsVisible();
  bool IsActive();
  bool IsRunning();
  bool IsUrgent();

  void Quit();
  void Stick();
  void UnStick();

  virtual bool ShowInSwitcher();
  virtual unsigned long long SwitcherPriority();

  std::vector<Window> RelatedXids() const;
  std::string NameForWindow(Window window) const;

protected:
  void UpdateIconGeometries(nux::Point3 center);
  void OnCenterStabilized(nux::Point3 center);
  void AddProperties(GVariantBuilder* builder);
  void OnAcceptDrop(unity::DndData& dnd_data);
  void OnDndEnter();
  void OnDndLeave();
  void OpenInstanceLauncherIcon(ActionArg arg);
  void ToggleSticky();

  nux::DndAction OnQueryAcceptDrop(unity::DndData& dnd_data);

  std::list<DbusmenuMenuitem*> GetMenus();
  std::set<std::string> ValidateUrisForLaunch(unity::DndData& dnd_data);

  const gchar* GetRemoteUri();
  std::string BamfName() const;

  bool HandlesSpread() { return true; }

private:
  void EnsureWindowState();
  void EnsureMenuItemsReady();
  void UpdateDesktopFile();
  void UpdateMenus();
  void UpdateDesktopQuickList();
  void FillSupportedTypes();

  void OpenInstanceWithUris(std::set<std::string> uris);
  void Focus(ActionArg arg);
  bool Spread(bool current_desktop, int state, bool force);

  void OnWindowMinimized(guint32 xid);
  void OnWindowMoved(guint32 xid);
  void OnLauncherHiddenChanged();

  bool OwnsWindow(Window w) const;

  const std::set<std::string>& GetSupportedTypes();

  static gboolean OnDndHoveredTimeout(gpointer data);


  glib::Object<BamfApplication> _bamf_app;
  Launcher* _launcher;
  bool _dnd_hovered;
  guint _dnd_hover_timer;

  bool _supported_types_filled;
  guint _fill_supported_types_id;
  guint _window_moved_id;

  std::string _remote_uri;
  std::string _desktop_file;
  std::set<std::string> _supported_types;
  std::map<std::string, glib::Object<DbusmenuClient>> _menu_clients;
  std::map<std::string, glib::Object<DbusmenuMenuitem>> _menu_items;
  std::map<std::string, glib::Object<DbusmenuMenuitem>> _menu_items_extra;
  glib::Object<IndicatorDesktopShortcuts> _desktop_shortcuts;
  glib::Object<DbusmenuMenuitem> _menu_desktop_shortcuts;
  glib::Object<GFileMonitor> _desktop_file_monitor;
  glib::SignalManager _gsignals;
};

}
}

#endif // BAMFLAUNCHERICON_H
