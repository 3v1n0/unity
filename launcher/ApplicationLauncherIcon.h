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

#ifndef APPLICATIONLAUNCHERICON_H
#define APPLICATIONLAUNCHERICON_H

#include <UnityCore/GLibSignal.h>
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/ConnectionManager.h>

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
  ApplicationLauncherIcon(ApplicationPtr const& app);
  virtual ~ApplicationLauncherIcon();

  virtual void ActivateLauncherIcon(ActionArg arg);

  std::string DesktopFile() const;

  bool IsSticky() const;
  bool IsActive() const;
  bool IsRunning() const;
  bool IsUrgent() const;

  virtual bool GetQuirk(Quirk quirk) const;

  virtual void Quit();
  virtual void AboutToRemove();

  virtual void Stick(bool save = true);
  virtual void UnStick();

  virtual bool ShowInSwitcher(bool current);
  virtual bool AllowDetailViewInSwitcher() const override;
  virtual uint64_t SwitcherPriority();

  virtual nux::Color BackgroundColor() const;

  WindowList Windows();
  std::vector<Window> WindowsOnViewport();
  std::vector<Window> WindowsForMonitor(int monitor);

  void PerformScroll(ScrollDirection direction, Time timestamp) override;

protected:
  void SetApplication(ApplicationPtr const& app);
  ApplicationPtr GetApplication() const;

  void Remove();
  void UpdateIconGeometries(std::vector<nux::Point3> center);
  void OnCenterStabilized(std::vector<nux::Point3> center);
  void AddProperties(GVariantBuilder* builder);
  void OnAcceptDrop(DndData const& dnd_data);
  void OnDndEnter();
  void OnDndHovered();
  void OnDndLeave();
  void OpenInstanceLauncherIcon(Time timestamp) override;
  void ToggleSticky();
  void LogUnityEvent(ApplicationEventType);
  bool IsFileManager();

  bool OnShouldHighlightOnDrag(DndData const& dnd_data);
  nux::DndAction OnQueryAcceptDrop(DndData const& dnd_data);

  MenuItemsVector GetMenus();

  std::string GetRemoteUri() const;

  bool HandlesSpread() { return true; }
  std::string GetName() const;

  void UpdateDesktopFile();
  void UpdateRemoteUri();
  std::string _desktop_file;

private:
  typedef unsigned long int WindowFilterMask;
  enum WindowFilter
  {
    MAPPED = (1 << 0),
    USER_VISIBLE = (1 << 1),
    ON_CURRENT_DESKTOP = (1 << 2),
    ON_ALL_MONITORS = (1 << 3),
  };

  void SetupApplicationSignalsConnections();
  void EnsureWindowState();
  void EnsureMenuItemsWindowsReady();
  void EnsureMenuItemsDefaultReady();
  void EnsureMenuItemsStaticQuicklist();
  void UpdateBackgroundColor();
  void UpdateDesktopQuickList();

  void OpenInstanceWithUris(std::set<std::string> const& uris, Time timestamp);
  void Focus(ActionArg arg);
  bool Spread(bool current_desktop, int state, bool force);

  void OnWindowMinimized(guint32 xid);
  void OnWindowMoved(guint32 xid);

  WindowList GetWindows(WindowFilterMask filter = 0, int monitor = -1);
  const std::set<std::string> GetSupportedTypes();
  WindowList GetWindowsOnCurrentDesktopInStackingOrder();
  ApplicationSubjectPtr GetSubject();

  ApplicationPtr app_;
  std::string _remote_uri;
  Time _startup_notification_timestamp;
  Time _last_scroll_timestamp;
  ScrollDirection _last_scroll_direction;
  unsigned int _progressive_scroll;
  std::set<std::string> _supported_types;
  std::vector<glib::Object<DbusmenuMenuitem>> _menu_items;
  std::vector<glib::Object<DbusmenuMenuitem>> _menu_items_windows;
  glib::Object<IndicatorDesktopShortcuts> _desktop_shortcuts;
  glib::Object<DbusmenuMenuitem> _menu_desktop_shortcuts;
  glib::Object<GFileMonitor> _desktop_file_monitor;
  glib::SignalManager _gsignals;

  bool use_custom_bg_color_;
  nux::Color bg_color_;

  connection::Manager signals_conn_;
};

}
}

#endif // BAMFLAUNCHERICON_H
