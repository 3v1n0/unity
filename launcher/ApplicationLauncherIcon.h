// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010-2015 Canonical Ltd
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

#ifndef APPLICATION_LAUNCHER_ICON_H
#define APPLICATION_LAUNCHER_ICON_H

#include <UnityCore/GLibSignal.h>
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/ConnectionManager.h>
#include <UnityCore/Variant.h>

#include <libindicator/indicator-desktop-shortcuts.h>

#include "WindowedLauncherIcon.h"

namespace unity
{
namespace launcher
{

class Launcher;

class ApplicationLauncherIcon : public virtual WindowedLauncherIcon
{
  NUX_DECLARE_OBJECT_TYPE(ApplicationLauncherIcon, WindowedLauncherIcon);
public:
  ApplicationLauncherIcon(ApplicationPtr const&);
  virtual ~ApplicationLauncherIcon();

  std::string DesktopFile() const;

  bool IsSticky() const override;
  bool IsUserVisible() const override;
  bool GetQuirk(Quirk quirk, int monitor = 0) const override;

  void Quit();

  void Stick(bool save = true) override;
  void UnStick() override;

protected:
  void SetApplication(ApplicationPtr const& app);
  ApplicationPtr GetApplication() const;

  WindowList GetManagedWindows() const override;

  void LogUnityEvent(ApplicationEventType);
  bool IsFileManager();
  void Remove();

  void AboutToRemove() override;
  bool AllowDetailViewInSwitcher() const override;
  uint64_t SwitcherPriority() override;
  void UpdateIconGeometries(std::vector<nux::Point3> const& centers) override;
  nux::Color BackgroundColor() const override;
  MenuItemsVector GetMenus() override;
  std::string GetRemoteUri() const override;

  void OpenInstanceLauncherIcon(Time timestamp) override;
  void OpenInstanceWithUris(std::set<std::string> const& uris, Time timestamp);
  void Focus(ActionArg arg) override;

  void OnAcceptDrop(DndData const&) override;
  bool OnShouldHighlightOnDrag(DndData const&) override;
  nux::DndAction OnQueryAcceptDrop(DndData const&) override;

  std::string GetName() const override;
  void AddProperties(debug::IntrospectionData&) override;

  void UnsetApplication();
  void SetupApplicationSignalsConnections();
  void EnsureMenuItemsWindowsReady();
  void EnsureMenuItemsDefaultReady();
  void EnsureMenuItemsStaticQuicklist();
  void UpdateBackgroundColor();
  void UpdateDesktopQuickList();
  void UpdateDesktopFile();
  void UpdateRemoteUri();
  void ToggleSticky();
  void OnApplicationClosed();

  const std::set<std::string> GetSupportedTypes();
  ApplicationSubjectPtr GetSubject();

  ApplicationPtr app_;
  std::string remote_uri_;
  Time startup_notification_timestamp_;
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

} // namespace launcher
} // namespace unity

#endif // APPLICATION_LAUNCHER_ICON_H
