/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Jason Smith <jason.smith@canonical.com>
 *              Tim Penhey <tim.penhey@canonical.com>
 *              Marco Trevisan (Treviño) <marco.trevisan@canonical.com>
 *
 */

#ifndef LAUNCHER_CONTROLLER_PRIVATE_H
#define LAUNCHER_CONTROLLER_PRIVATE_H

#include <Nux/Nux.h>
#include <UnityCore/GLibDBusServer.h>

#include "AbstractLauncherIcon.h"
#include "DeviceLauncherSection.h"
#ifdef USE_X11
#include "EdgeBarrierController.h"
#endif
#include "LauncherController.h"
#include "Launcher.h"
#include "LauncherEntryRemote.h"
#include "LauncherEntryRemoteModel.h"
#include "LauncherModel.h"
#include "SoftwareCenterLauncherIcon.h"
#include "unity-shared/UBusWrapper.h"
#include "XdndManager.h"

namespace unity
{
namespace launcher
{

class Controller::Impl : public sigc::trackable
{
public:
  Impl(Controller* parent, XdndManager::Ptr const& xdnd_manager, ui::EdgeBarrierController::Ptr const& edge_barriers);
  ~Impl();

  void UpdateNumWorkspaces(int workspaces);

  Launcher* CreateLauncher();

  void SaveIconsOrder();
  void SortAndUpdate();

  nux::ObjectPtr<Launcher> CurrentLauncher();

  template<typename IconType>
  int GetLastIconPriority(std::string const& favorite_uri = "", bool sticky = false);
  void AddFavoriteKeepingOldPosition(FavoriteList& icons, std::string const& icon_uri) const;

  void OnIconRemoved(AbstractLauncherIcon::Ptr const& icon);
  void OnDeviceIconAdded(AbstractLauncherIcon::Ptr const& icon);

  void OnLauncherAddRequest(std::string const& icon_uri, AbstractLauncherIcon::Ptr const& before);
  void OnLauncherAddRequestSpecial(std::string const& path, std::string const& aptdaemon_trans_id,
                                   std::string const& icon_path, int icon_x, int icon_y, int icon_size);
  void OnLauncherUpdateIconStickyState(std::string const& desktop_file, bool sticky);
  void OnLauncherRemoveRequest(AbstractLauncherIcon::Ptr const& icon);

  void OnLauncherEntryRemoteAdded(LauncherEntryRemote::Ptr const& entry);
  void OnLauncherEntryRemoteRemoved(LauncherEntryRemote::Ptr const& entry);

  void OnFavoriteStoreFavoriteAdded(std::string const& entry, std::string const& pos, bool before);
  void OnFavoriteStoreFavoriteRemoved(std::string const& entry);
  void ResetIconPriorities();

  void SendHomeActivationRequest();

  int MonitorWithMouse();

  void RegisterIcon(AbstractLauncherIcon::Ptr const& icon, int priority = std::numeric_limits<int>::min());

  AbstractLauncherIcon::Ptr CreateFavoriteIcon(std::string const& icon_uri, bool emit_signal = false);
  AbstractLauncherIcon::Ptr GetIconByUri(std::string const& icon_uri);
  SoftwareCenterLauncherIcon::Ptr CreateSCLauncherIcon(std::string const& file_path, std::string const& aptdaemon_trans_id, std::string const& icon_path);

  void SetupIcons();
  void MigrateFavorites();
  void AddRunningApps();
  void AddDevices();

  void EnsureLaunchers(int primary, std::vector<nux::Geometry> const& monitors);

  void OnScreenChanged(int primary_monitor, std::vector<nux::Geometry>& monitors);

  void OnWindowFocusChanged (guint32 xid);

  void OnApplicationStarted(ApplicationPtr const& app);

  void ReceiveMouseDownOutsideArea(int x, int y, unsigned long button_flags, unsigned long key_flags);

  void ReceiveLauncherKeyPress(unsigned long eventType,
                               unsigned long keysym,
                               unsigned long state,
                               const char* character,
                               unsigned short keyCount);

  void OpenQuicklist();

  void OnDndStarted(std::string const& data, int monitor);
  void OnDndFinished();
  void OnDndMonitorChanged(std::string const& data, int old_monitor, int new_monitor);
  GVariant* OnDBusMethodCall(std::string const& method, GVariant *parameters);

  Controller* parent_;
  LauncherModel::Ptr model_;
  nux::ObjectPtr<Launcher> launcher_;
  nux::ObjectPtr<Launcher> keyboard_launcher_;
  XdndManager::Ptr xdnd_manager_;
  DeviceLauncherSection  device_section_;
  LauncherEntryRemoteModel remote_model_;
  AbstractLauncherIcon::Ptr expo_icon_;
  AbstractLauncherIcon::Ptr desktop_icon_;

#ifdef USE_X11
  ui::EdgeBarrierController::Ptr edge_barriers_;
#endif

  LauncherList launchers;

  unsigned sort_priority_;
  bool launcher_open;
  bool launcher_keynav;
  bool launcher_grabbed;
  bool reactivate_keynav;
  int reactivate_index;
  bool keynav_restore_window_;
  int launcher_key_press_time_;

  glib::DBusServer dbus_server_;
  glib::SourceManager sources_;
  UBusManager ubus;

  connection::Wrapper launcher_key_press_connection_;
  connection::Wrapper launcher_event_outside_connection_;
  connection::Wrapper average_color_connection_;
};

} // launcher namespace
} // unity namespace

#endif
