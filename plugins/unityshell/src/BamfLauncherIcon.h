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
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libbamf/libbamf.h>
#include <UnityCore/GLibSignal.h>
#include <UnityCore/GLibWrapper.h>

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

  bool IsSticky();
  bool IsVisible();
  bool IsActive();
  bool IsRunning();
  bool IsUrgent();

  void Quit();
  void Stick();
  void UnStick();

  virtual bool ShowInSwitcher();
  virtual unsigned long long SwitcherPriority();

  std::vector<Window> RelatedXids();
  std::string NameForWindow (Window window);

protected:
  void UpdateIconGeometries(nux::Point3 center);
  void OnCenterStabilized(nux::Point3 center);
  void AddProperties(GVariantBuilder* builder);
  void OnAcceptDrop(unity::DndData& dnd_data);
  void OnDndEnter();
  void OnDndLeave();
  void OpenInstanceLauncherIcon(ActionArg arg);

  nux::DndAction OnQueryAcceptDrop(unity::DndData& dnd_data);

  std::list<DbusmenuMenuitem*> GetMenus();
  std::set<std::string> ValidateUrisForLaunch(unity::DndData& dnd_data);

  const gchar* GetRemoteUri();
  std::string BamfName();

  bool HandlesSpread() { return true; }

private:
  void EnsureWindowState();

  void UpdateDesktopFile();
  void UpdateMenus();
  void UpdateDesktopQuickList();

  void OpenInstanceWithUris(std::set<std::string> uris);
  void Focus(ActionArg arg);
  bool Spread(bool current_desktop, int state, bool force);

  void EnsureMenuItemsReady();

  void OnWindowMinimized(guint32 xid);
  void OnWindowMoved(guint32 xid);
  void OnLauncherHiddenChanged();

  bool OwnsWindow(Window w);

  const std::set<std::string>& GetSupportedTypes();

  static void OnQuit(DbusmenuMenuitem* item, int time, BamfLauncherIcon* self);
  static void OnLaunch(DbusmenuMenuitem* item, int time, BamfLauncherIcon* self);
  static void OnTogglePin(DbusmenuMenuitem* item, int time, BamfLauncherIcon* self);

  static gboolean OnDndHoveredTimeout(gpointer data);
  static gboolean FillSupportedTypes(gpointer data);


  glib::Object<BamfApplication> _bamf_app;
  Launcher* _launcher;
  const gchar* _desktop_file;
  gchar* _remote_uri;
  bool _dnd_hovered;
  guint _dnd_hover_timer;

  bool _supported_types_filled;
  guint _fill_supported_types_id;
  guint32 _window_moved_id;

  std::set<std::string> _supported_types;
  std::map<std::string, DbusmenuClient*> _menu_clients;
  std::map<std::string, DbusmenuMenuitem*> _menu_items;
  std::map<std::string, DbusmenuMenuitem*> _menu_items_extra;
  std::map<std::string, gulong> _menu_callbacks;
  glib::Object<DbusmenuMenuitem> _menu_desktop_shortcuts;
  glib::Object<GFileMonitor> _desktop_file_monitor;
  glib::SignalManager _gsignals;
};

}
}

#endif // BAMFLAUNCHERICON_H
