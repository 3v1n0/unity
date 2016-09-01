// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 3 -*-
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
  auto const& busname = test ? "org.freedesktop.systemd1" : "com.canonical.Unity.Test.Systemd";
  systemd_proxy_ = std::make_shared<unity::glib::DBusProxy>(busname, "/org/freedesktop/systemd1", "org.freedesktop.systemd1.Manager");
}

void SystemdWrapper::Impl::Start(std::string const& name)
{
  if (IsConnected())
    systemd_proxy_->Call("StartUnit", g_variant_new("(ss)", name.c_str(), "replace"));
}

void SystemdWrapper::Impl::Stop(std::string const& name)
{
  if (IsConnected())
    systemd_proxy_->Call("StopUnit", g_variant_new("(ss)", name.c_str(), "replace"));
}

bool SystemdWrapper::Impl::IsConnected()
{
  return systemd_proxy_->IsConnected();
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

bool SystemdWrapper::IsConnected()
{
  return pimpl_->IsConnected();
}

}
