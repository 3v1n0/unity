// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright © 2016 Canonical Ltd
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
* Authored by: Ted Gould <ted@canonical.com>
*/

#include "SystemdWrapper.h"

#include <UnityCore/GLibDBusProxy.h>

namespace unity
{

//
// Start private implementation
//

class SystemdWrapper::Impl
{
public:
  Impl(bool test);

  void Start(std::string const& name);
  void Stop(std::string const& name);
  bool IsConnected();

private:
  glib::DBusProxy::Ptr systemd_proxy_;
};

SystemdWrapper::Impl::Impl(bool test)
{
  auto const& busname = test ? "com.canonical.Unity.Test.Systemd" : "org.freedesktop.systemd1";
  auto flags = static_cast<GDBusProxyFlags>(G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES |
                                            G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS);
  systemd_proxy_ = std::make_shared<unity::glib::DBusProxy>(busname, "/org/freedesktop/systemd1",
                                                            "org.freedesktop.systemd1.Manager",
                                                            G_BUS_TYPE_SESSION, flags);
}

void SystemdWrapper::Impl::Start(std::string const& name)
{
  systemd_proxy_->Call("StartUnit", g_variant_new("(ss)", name.c_str(), "replace"));
}

void SystemdWrapper::Impl::Stop(std::string const& name)
{
  systemd_proxy_->Call("StopUnit", g_variant_new("(ss)", name.c_str(), "replace"));
}

//
// End private implementation
//

SystemdWrapper::SystemdWrapper()
  : pimpl_(new Impl(false))
{}

SystemdWrapper::SystemdWrapper(SystemdWrapper::TestMode const& tm)
  : pimpl_(new Impl(true))
{}

SystemdWrapper::~SystemdWrapper()
{}

void SystemdWrapper::Start(std::string const& name)
{
  pimpl_->Start(name);
}

void SystemdWrapper::Stop(std::string const& name)
{
  pimpl_->Stop(name);
}

}
