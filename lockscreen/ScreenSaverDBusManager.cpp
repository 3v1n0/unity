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
    <method name="SimulateUserActivity" />
    <method name="GetActive">
      <arg type="b" direction="out" name="value" />
    </method>
    <method name="GetActiveTime">
      <arg type="u" direction="out" name="seconds" />
    </method>
    <method name="SetActive">
      <arg type="b" direction="in" name="value" />
    </method>
    <method name="ShowMessage">
      <arg name="summary" direction="in" type="s"/>
      <arg name="body" direction="in" type="s"/>
      <arg name="icon" direction="in" type="s"/>
    </method>
    <signal name="ActiveChanged">
      <arg name="new_value" type="b" />
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
    if (method == "Lock")
    {
      session_->LockScreen();
    }
    else if (method == "SimulateUserActivity")
    {
    }
    else if (method == "GetActive")
    {
      return g_variant_new("(b)", TRUE);
    }
    else if (method == "GetActiveTime")
    {
    }
    else if (method == "SetActive")
    {}
    else if (method == "ShowMessage")
    {}


    connections_.Add(session_->presence_status_changed.connect([this] (bool value) {
      if (Settings::Instance().idle_activation_enabled())
          object_->EmitSignal("ActiveChanged", g_variant_new("(b)", value));
    }));

    return nullptr;
  });
}

} // lockscreen
} // unity
