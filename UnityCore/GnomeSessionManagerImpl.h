// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2013 Canonical Ltd
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
* Authored by: Marco Trevisan (Treviño) <marco@ubuntu.com>
*/

#ifndef UNITY_GNOME_SESSION_MANAGER_IMPL_H
#define UNITY_GNOME_SESSION_MANAGER_IMPL_H

#include "GnomeSessionManager.h"
#include "GLibDBusProxy.h"
#include "GLibDBusServer.h"

namespace unity
{
namespace session
{

namespace shell
{
  enum class Action : unsigned
  {
    LOGOUT = 0,
    SHUTDOWN,
    REBOOT,
    NONE
  };
}

struct GnomeManager::Impl
{
  Impl(GnomeManager* parent, bool test_mode = false);
  ~Impl();

  void ConfirmLogout();
  void ConfirmReboot();
  void ConfirmShutdown();
  void CancelAction();
  void ClosedDialog();
  void EnsureCancelPendingAction();

  GVariant* OnShellMethodCall(std::string const& method, GVariant* parameters);
  void CallGnomeSessionMethod(std::string const& method, GVariant* parameters = nullptr,
                              glib::DBusProxy::CallFinishedCallback const& cb = nullptr);
  void CallUPowerMethod(std::string const& method, glib::DBusProxy::ReplyCallback const& cb = nullptr);
  void CallLogindMethod(std::string const& method, GVariant* parameters = nullptr, glib::DBusProxy::CallFinishedCallback const& cb = nullptr);
  void CallConsoleKitMethod(std::string const& method, GVariant* parameters = nullptr);
  bool InteractiveMode();

  GnomeManager* manager_;
  bool test_mode_;
  bool can_shutdown_;
  bool can_suspend_;
  bool can_hibernate_;

  shell::Action pending_action_;
  glib::DBusServer shell_server_;
  glib::DBusObject::Ptr shell_object_;
};

} // namespace session
} // namespace unity

#endif //UNITY_GNOME_SESSION_MANAGER_IMPL_H
