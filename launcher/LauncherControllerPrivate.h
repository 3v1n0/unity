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

#include "AbstractLauncherIcon.h"
#include "DeviceLauncherSection.h"
#include "EdgeBarrierController.h"
#include "LauncherController.h"
#include "Launcher.h"
#include "LauncherEntryRemote.h"
#include "LauncherEntryRemoteModel.h"
#include "LauncherModel.h"
#include "SoftwareCenterLauncherIcon.h"
#include "unity-shared/UBusWrapper.h"
#include "VolumeMonitorWrapper.h"

namespace unity
{
namespace launcher
{

class Controller::Impl
{
public:
  Impl(Display* display, Controller* parent);
  ~Impl();

  void UpdateNumWorkspaces(int workspaces);

  Launcher* CreateLauncher(int monitor);

  void Save();
  void SortAndUpdate();

  nux::ObjectPtr<Launcher> CurrentLauncher();

  void OnIconAdded(AbstractLauncherIcon::Ptr icon);
  void OnIconRemoved(AbstractLauncherIcon::Ptr icon);

  void OnLauncherAddRequest(char* path, AbstractLauncherIcon::Ptr before);
  void OnLauncherAddRequestSpecial(std::string const& path, std::string const& aptdaemon_trans_id,
                                   std::string const& icon_path, int icon_x, int icon_y, int icon_size);
  void OnLauncherRemoveRequest(AbstractLauncherIcon::Ptr icon);
  void OnSCIconAnimationComplete(AbstractLauncherIcon::Ptr icon);

  void OnLauncherEntryRemoteAdded(LauncherEntryRemote::Ptr const& entry);
  void OnLauncherEntryRemoteRemoved(LauncherEntryRemote::Ptr const& entry);

  void OnFavoriteStoreFavoriteAdded(std::string const& entry, std::string const& pos, bool before);
  void OnFavoriteStoreFavoriteRemoved(std::string const& entry);
  void OnFavoriteStoreReordered();


  void InsertExpoAction();
  void RemoveExpoAction();

  void InsertDesktopIcon();
  void RemoveDesktopIcon();

  void SendHomeActivationRequest();

  int MonitorWithMouse();

  void InsertTrash();

  void RegisterIcon(AbstractLauncherIcon::Ptr icon);

  AbstractLauncherIcon::Ptr CreateFavorite(const char* file_path);

  SoftwareCenterLauncherIcon::Ptr CreateSCLauncherIcon(std::string const& file_path, std::string const& aptdaemon_trans_id, std::string const& icon_path);

  void SetupBamf();

  void EnsureLaunchers(int primary, std::vector<nux::Geometry> const& monitors);

  void OnExpoActivated();

  void OnScreenChanged(int primary_monitor, std::vector<nux::Geometry>& monitors);

  void OnWindowFocusChanged (guint32 xid);

  void OnViewOpened(BamfMatcher* matcher, BamfView* view);

  void ReceiveMouseDownOutsideArea(int x, int y, unsigned long button_flags, unsigned long key_flags);

  void ReceiveLauncherKeyPress(unsigned long eventType,
                               unsigned long keysym,
                               unsigned long state,
                               const char* character,
                               unsigned short keyCount);

  static void OnBusAcquired(GDBusConnection* connection, const gchar* name, gpointer user_data);
  static void OnDBusMethodCall(GDBusConnection* connection, const gchar* sender, const gchar* object_path,
                               const gchar* interface_name, const gchar* method_name,
                               GVariant* parameters, GDBusMethodInvocation* invocation,
                               gpointer user_data);

  static GDBusInterfaceVTable interface_vtable;

  Controller* parent_;
  LauncherModel::Ptr model_;
  nux::ObjectPtr<Launcher> launcher_;
  nux::ObjectPtr<Launcher> keyboard_launcher_;
  int                    sort_priority_;
  AbstractVolumeMonitorWrapper::Ptr volume_monitor_;
  DeviceLauncherSection  device_section_;
  LauncherEntryRemoteModel remote_model_;
  AbstractLauncherIcon::Ptr expo_icon_;
  AbstractLauncherIcon::Ptr desktop_icon_;
  int                    num_workspaces_;
  bool                   show_desktop_icon_;
  Display*               display_;

  bool                   launcher_open;
  bool                   launcher_keynav;
  bool                   launcher_grabbed;
  bool                   reactivate_keynav;
  int                    reactivate_index;
  bool                   keynav_restore_window_;
  int                    launcher_key_press_time_;
  unsigned int           dbus_owner_;

  ui::EdgeBarrierController edge_barriers_;

  LauncherList launchers;

  glib::Object<BamfMatcher> matcher_;
  glib::Signal<void, BamfMatcher*, BamfView*> view_opened_signal_;
  glib::SourceManager sources_;
  UBusManager ubus;

  sigc::connection on_expoicon_activate_connection_;
  sigc::connection launcher_key_press_connection_;
  sigc::connection launcher_event_outside_connection_;
};

} // launcher namespace
} // unity namespace

#endif
