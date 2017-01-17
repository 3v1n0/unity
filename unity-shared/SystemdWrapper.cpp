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
  void CallMethod(std::string const& method, std::string const& unit);

private:
  bool test_mode_;
};

SystemdWrapper::Impl::Impl(bool test)
  : test_mode_(test)
{}

void SystemdWrapper::Impl::CallMethod(std::string const& method, std::string const& unit)
{
  auto const& busname = test_mode_ ? "com.canonical.Unity.Test.Systemd" : "org.freedesktop.systemd1";
  auto flags = static_cast<GDBusProxyFlags>(G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES |
                                            G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS);
  auto proxy = std::make_shared<glib::DBusProxy>(busname, "/org/freedesktop/systemd1",
                                                 "org.freedesktop.systemd1.Manager",
                                                 G_BUS_TYPE_SESSION, flags);

  proxy->CallBegin(method, g_variant_new("(ss)", unit.c_str(), "replace"), [proxy] (GVariant*, glib::Error const& e) {});
}

void SystemdWrapper::Impl::Start(std::string const& name)
{
  CallMethod("StartUnit", name);
}

void SystemdWrapper::Impl::Stop(std::string const& name)
{
  CallMethod("StopUnit", name);
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
