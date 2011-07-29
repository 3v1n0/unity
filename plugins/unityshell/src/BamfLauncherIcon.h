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
 * Authored by: Jason Smith <jason.smith@canonical.com>
 */

#ifndef BAMFLAUNCHERICON_H
#define BAMFLAUNCHERICON_H

/* Compiz */
#include <core/core.h>

#include <Nux/BaseWindow.h>
#include <NuxCore/Math/MathInc.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libbamf/libbamf.h>
#include <sigc++/sigc++.h>

#include "SimpleLauncherIcon.h"

class Launcher;

class BamfLauncherIcon : public SimpleLauncherIcon
{
public:
  BamfLauncherIcon(Launcher* IconManager, BamfApplication* app, CompScreen* screen);
  virtual ~BamfLauncherIcon();

  const char* DesktopFile();
  bool IsSticky();
  void UnStick();

  virtual void Activate(ActionArg arg);
  void ActivateLauncherIcon(ActionArg arg);

  virtual bool ShowInSwitcher();
  virtual unsigned int SwitcherPriority();

protected:
  std::list<DbusmenuMenuitem*> GetMenus();

  void UpdateIconGeometries(nux::Point3 center);
  void OnCenterStabilized(nux::Point3 center);

  void OnLauncherHiddenChanged();

  void AddProperties(GVariantBuilder* builder);

  const gchar* GetRemoteUri();

  nux::DndAction OnQueryAcceptDrop(std::list<char*> uris);
  void OnAcceptDrop(std::list<char*> uris);
  void OnDndEnter();
  void OnDndLeave();

  void OpenInstanceLauncherIcon();

  std::list<char*> ValidateUrisForLaunch(std::list<char*> uris);

  const char* BamfName();

private:
  BamfApplication* m_App;
  CompScreen* m_Screen;
  Launcher* _launcher;
  std::map<std::string, DbusmenuClient*> _menu_clients;
  std::map<std::string, DbusmenuMenuitem*> _menu_items;
  std::map<std::string, gulong> _menu_callbacks;
  DbusmenuMenuitem* _menu_desktop_shortcuts;
  gchar* _remote_uri;
  bool _dnd_hovered;
  guint _dnd_hover_timer;

  gchar* _cached_desktop_file;
  gchar* _cached_name;


  GFileMonitor* _desktop_file_monitor;
  gulong _on_desktop_file_changed_handler_id;

  void EnsureWindowState();

  void UpdateMenus();

  void OpenInstanceWithUris(std::list<char*> uris);
  void Focus();
  bool Spread(int state, bool force);

  void EnsureMenuItemsReady();

  void OnWindowMinimized(guint32 xid);
  void OnViewPortSwitchEnded();
  bool OwnsWindow(Window w);

  static void OnClosed(BamfView* view, gpointer data);
  static void OnUserVisibleChanged(BamfView* view, gboolean visible, gpointer data);
  static void OnActiveChanged(BamfView* view, gboolean active, gpointer data);
  static void OnRunningChanged(BamfView* view, gboolean running, gpointer data);
  static void OnUrgentChanged(BamfView* view, gboolean urgent, gpointer data);
  static void OnChildAdded(BamfView* view, BamfView* child, gpointer data);
  static void OnChildRemoved(BamfView* view, BamfView* child, gpointer data);

  static void OnQuit(DbusmenuMenuitem* item, int time, BamfLauncherIcon* self);
  static void OnLaunch(DbusmenuMenuitem* item, int time, BamfLauncherIcon* self);
  static void OnTogglePin(DbusmenuMenuitem* item, int time, BamfLauncherIcon* self);

  static void OnDesktopFileChanged(GFileMonitor*        monitor,
                                   GFile*               file,
                                   GFile*               other_file,
                                   GFileMonitorEvent    event_type,
                                   gpointer             data);

  static gboolean OnDndHoveredTimeout(gpointer data);
};

#endif // BAMFLAUNCHERICON_H

