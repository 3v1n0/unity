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
#include "ScreenSaverDBusManagerImpl.h"

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
const std::string TEST_NAME = "org.gnome.Test.ScreenSaver";
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
    <method name="SimulateUserActivity" />
    <signal name="ActiveChanged">
      <arg name="new_value" type="b" />
    </signal>
  </interface>
</node>)";
}

//
// Start Private Implementation
//

DBusManager::Impl::Impl(DBusManager* manager, session::Manager::Ptr const& session, bool test_mode)
  : manager_(manager)
  , session_(session)
  , test_mode_(test_mode)
  , object_(std::make_shared<glib::DBusObject>(dbus::INTROSPECTION_XML, dbus::INTERFACE))
  , time_(0)
{
  manager_->active = false;
  manager_->active.changed.connect(sigc::mem_fun(this, &DBusManager::Impl::SetActive));

  // This is a workaround we use to fallback to use gnome-screensaver if the screen reader is enabled
  Settings::Instance().use_legacy.changed.connect(sigc::hide(sigc::mem_fun(this, &Impl::EnsureService)));

  object_->SetMethodsCallsHandler([this] (std::string const& method, GVariant* parameters) -> GVariant* {
    if (method == "Lock")
    {
      session_->LockScreen();
    }
    else if (method == "GetActive")
    {
      return g_variant_new("(b)", manager_->active() ? TRUE : FALSE);
    }
    else if (method == "GetActiveTime")
    {
      return g_variant_new("(u)", time_ ? (time(nullptr) - time_) : 0);
    }
    else if (method == "SetActive")
    {
      if (glib::Variant(parameters).GetBool())
        session_->ScreenSaverActivate();
      else
        session_->ScreenSaverDeactivate();
    }
    else if (method == "SimulateUserActivity")
    {
      manager_->simulate_activity.emit();
    }

    return nullptr;
  });

  EnsureService();
}

void DBusManager::Impl::EnsureService()
{
  if (!Settings::Instance().use_legacy())
  {
    if (!server_)
    {
      server_ = std::make_shared<glib::DBusServer>(test_mode_ ? dbus::TEST_NAME : dbus::NAME,
                                                   G_BUS_TYPE_SESSION, G_BUS_NAME_OWNER_FLAGS_REPLACE);
      server_->AddObject(object_, dbus::OBJECT_PATH);
    }
  }
  else
  {
    server_.reset();

    // Let's make GS to restart...
    auto proxy = std::make_shared<glib::DBusProxy>("org.gnome.ScreenSaver", "/org/gnome/ScreenSaver", "org.gnome.ScreenSaver");
    proxy->CallBegin("SimulateUserActivity", nullptr, [proxy] (GVariant*, glib::Error const&) {});
  }
}

void DBusManager::Impl::SetActive(bool active)
{
  time_ = active ? time(nullptr) : 0;
  object_->EmitSignal("ActiveChanged", g_variant_new("(b)", active ? TRUE : FALSE));
}

//
// End private implemenation
//

DBusManager::DBusManager(session::Manager::Ptr const& session)
  : impl_(new Impl(this, session, /* test_mode */ false))
{}

DBusManager::DBusManager(session::Manager::Ptr const& session, DBusManager::TestMode const&)
  : impl_(new Impl(this, session, /* test_mode */ true))
{}

DBusManager::~DBusManager()
{}

} // lockscreen
} // unity
