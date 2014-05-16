// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2014 Canonical Ltd
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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#include "SessionDBusManager.h"

namespace unity
{
namespace session
{
namespace dbus
{
const std::string NAME = "com.canonical.Unity";
const std::string INTERFACE = "com.canonical.Unity.Session";
const std::string OBJECT_PATH = "/com/canonical/Unity/Session";
const std::string INTROSPECTION_XML =
R"(<node>
  <interface name="com.canonical.Unity.Session">
    <method name="RealName">
      <arg type="s" direction="out" name="realname" />
    </method>
    <method name="UserName">
      <arg type="s" direction="out" name="username" />
    </method>
    <method name="HostName">
      <arg type="s" direction="out" name="hostname" />
    </method>
    <method name="Lock" />
    <method name="PromptLock" />
    <method name="ActivateScreenSaver" />
    <method name="DeactivateScreenSaver" />
    <method name="Logout" />
    <method name="RequestLogout" />
    <method name="Reboot" />
    <method name="RequestReboot" />
    <method name="Shutdown" />
    <method name="RequestShutdown" />
    <method name="Suspend" />
    <method name="Hibernate" />
    <method name="CancelAction" />
    <method name="IsLocked">
      <arg type="b" direction="out" name="is_locked" />
    </method>
    <method name="CanLock">
      <arg type="b" direction="out" name="can_lock" />
    </method>
    <method name="CanShutdown">
      <arg type="b" direction="out" name="can_shutdown" />
    </method>
    <method name="CanSuspend">
      <arg type="b" direction="out" name="can_suspend" />
    </method>
    <method name="CanHibernate">
      <arg type="b" direction="out" name="can_hibernate" />
    </method>

    <signal name="LockRequested" />
    <signal name="Locked" />
    <signal name="UnlockRequested" />
    <signal name="Unlocked" />
    <signal name="LogoutRequested">
      <arg type="b" name="have_inhibitors" />
    </signal>
    <signal name="RebootRequested">
      <arg type="b" name="have_inhibitors" />
    </signal>
    <signal name="ShutdownRequested">
      <arg type="b" name="have_inhibitors" />
    </signal>
  </interface>
</node>)";
}

DBusManager::DBusManager(session::Manager::Ptr const& session)
  : session_(session)
  , server_(dbus::NAME)
{
  server_.AddObjects(dbus::INTROSPECTION_XML, dbus::OBJECT_PATH);
  object_ = server_.GetObject(dbus::INTERFACE);
  object_->SetMethodsCallsHandler([this] (std::string const& method, GVariant*) -> GVariant* {
    if (method == "RealName")
    {
      return g_variant_new("(s)", session_->RealName().c_str());
    }
    else if (method == "UserName")
    {
      return g_variant_new("(s)", session_->UserName().c_str());
    }
    else if (method == "HostName")
    {
      return g_variant_new("(s)", session_->HostName().c_str());
    }
    else if (method == "Lock")
    {
      session_->LockScreen();
    }
    else if (method == "PromptLock")
    {
      session_->PromptLockScreen();
    }
    else if (method == "ActivateScreenSaver")
    {
      session_->ScreenSaverActivate();
    }
    else if (method == "DeactivateScreenSaver")
    {
      session_->ScreenSaverDeactivate();
    }
    else if (method == "Logout")
    {
      session_->Logout();
    }
    else if (method == "RequestLogout")
    {
      session_->logout_requested.emit(session_->HasInhibitors());
    }
    else if (method == "Reboot")
    {
      session_->Reboot();
    }
    else if (method == "RequestReboot")
    {
      session_->reboot_requested.emit(session_->HasInhibitors());
    }
    else if (method == "Shutdown")
    {
      session_->Shutdown();
    }
    else if (method == "RequestShutdown")
    {
      session_->shutdown_requested.emit(session_->HasInhibitors());
    }
    else if (method == "Suspend")
    {
      session_->Suspend();
    }
    else if (method == "Hibernate")
    {
      session_->Hibernate();
    }
    else if (method == "CancelAction")
    {
      session_->CancelAction();
      session_->cancel_requested.emit();
    }
    else if (method == "IsLocked")
    {
      return g_variant_new("(b)", session_->is_locked() != false);
    }
    else if (method == "CanLock")
    {
      return g_variant_new("(b)", session_->CanLock() != false);
    }
    else if (method == "CanShutdown")
    {
      return g_variant_new("(b)", session_->CanShutdown() != false);
    }
    else if (method == "CanSuspend")
    {
      return g_variant_new("(b)", session_->CanSuspend() != false);
    }
    else if (method == "CanHibernate")
    {
      return g_variant_new("(b)", session_->CanHibernate() != false);
    }

    return nullptr;
  });

  connections_.Add(session_->lock_requested.connect([this] {
    object_->EmitSignal("LockRequested");
  }));
  connections_.Add(session_->prompt_lock_requested.connect([this] {
    object_->EmitSignal("LockRequested");
  }));
  connections_.Add(session_->locked.connect([this] {
    object_->EmitSignal("Locked");
  }));
  connections_.Add(session_->unlock_requested.connect([this] {
    object_->EmitSignal("UnlockRequested");
  }));
  connections_.Add(session_->unlocked.connect([this] {
    object_->EmitSignal("Unlocked");
  }));
  connections_.Add(session_->logout_requested.connect([this] (bool inhibitors) {
    object_->EmitSignal("LogoutRequested", g_variant_new("(b)", inhibitors));
  }));
  connections_.Add(session_->reboot_requested.connect([this] (bool inhibitors) {
    object_->EmitSignal("RebootRequested", g_variant_new("(b)", inhibitors));
  }));
  connections_.Add(session_->shutdown_requested.connect([this] (bool inhibitors) {
    object_->EmitSignal("ShutdownRequested", g_variant_new("(b)", inhibitors));
  }));
}

} // session
} // unity
