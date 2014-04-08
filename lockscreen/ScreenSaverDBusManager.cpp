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
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 */

#include "ScreenSaverDBusManager.h"

#include "LockScreenSettings.h"
#include "GLibDBusProxy.h"
#include "Variant.h"

namespace unity
{
namespace lockscreen
{
namespace dbus
{
const std::string NAME = "org.gnome.ScreenSaver";
const std::string INTERFACE = "org.gnome.ScreenSaver";
const std::string OBJECT_PATH = "/org/gnome/ScreenSaver";
const std::string INTROSPECTION_XML =
R"(<node>
  <interface name="org.gnome.ScreenSaver">
    <method name="Lock" />
    <method name="GetActive">
      <arg type="b" direction="out" name="value" />
    </method>
    <method name="GetActiveTime">
      <arg type="u" direction="out" name="seconds" />
    </method>
    <method name="SetActive">
      <arg type="b" direction="in" name="value" />
    </method>
    <signal name="ActiveChanged">
      <arg name="new_value" type="b" />
    </signal>
  </interface>
</node>)";
}

DBusManager::DBusManager(session::Manager::Ptr const& session)
  : session_(session)
  , object_(std::make_shared<glib::DBusObject>(dbus::INTROSPECTION_XML, dbus::INTERFACE))
  , active_(false)
  , time_(0)
{
  // This is a workaround we use to fallback to use gnome-screensaver if the screen reader is enabled
  Settings::Instance().use_legacy.changed.connect(sigc::hide(sigc::mem_fun(this, &DBusManager::EnsureService)));

  object_->SetMethodsCallsHandler([this] (std::string const& method, GVariant* variant) -> GVariant* {
    if (method == "Lock")
    {
      session_->LockScreen();
    }
    else if (method == "GetActive")
    {
      return g_variant_new("(b)", active_ ? TRUE : FALSE);
    }
    else if (method == "GetActiveTime")
    {
      if (time_)
        return g_variant_new("(u)", time(NULL) - time_);
      else
        return g_variant_new("(u)", 0);
    }
    else if (method == "SetActive")
    {
      session_->presence_status_changed.emit(glib::Variant(variant).GetBool());
    }

    return nullptr;
  });

  EnsureService();
}

void DBusManager::EnsureService()
{
  if (!Settings::Instance().use_legacy())
  {
    if (!server_)
    {
      g_spawn_command_line_async("killall -q gnome-screensaver", nullptr);
      server_ = std::make_shared<glib::DBusServer>(dbus::NAME);
      server_->AddObject(object_, dbus::OBJECT_PATH);
    }
  }
  else
  {
    server_.reset();
    auto proxy = std::make_shared<glib::DBusProxy>("org.gnome.ScreenSaver", "/org/gnome/ScreenSaver", "org.gnome.ScreenSaver");
    // By passing the proxy to the lambda we ensure that it will stay alive
    // until we get the last callback.
    proxy->CallBegin("SimulateUserActivity", nullptr, [proxy] (GVariant*, glib::Error const&) {});
  }
}

void DBusManager::SetActive(bool active)
{
  if (active_ == active)
    return;

  active_ = active;

  if (active_)
    time_ = time(nullptr);
  else
    time_ = 0;

  object_->EmitSignal("ActiveChanged", g_variant_new("(b)", active_ ? TRUE : FALSE));
}

} // lockscreen
} // unity
