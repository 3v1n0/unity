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

#include "UpstartWrapper.h"

#include <UnityCore/GLibDBusProxy.h>

namespace unity
{

//
// Start private implementation
//

class UpstartWrapper::Impl
{
public:
  Impl();

  void Emit(std::string const& name);

private:
  glib::DBusProxy::Ptr upstart_proxy_;
};

UpstartWrapper::Impl::Impl()
{
  upstart_proxy_ = std::make_shared<unity::glib::DBusProxy>("com.ubuntu.Upstart",
                                                            "/com/ubuntu/Upstart", 
                                                            "com.ubuntu.Upstart0_6");
}

void UpstartWrapper::Impl::Emit(std::string const& name)
{
  upstart_proxy_->Call("EmitEvent", g_variant_new("(sasb)", name.c_str(), nullptr, 0));
}

//
// End private implementation
//

UpstartWrapper::UpstartWrapper()
  : pimpl_(new Impl)
{}

UpstartWrapper::~UpstartWrapper()
{}

void UpstartWrapper::Emit(std::string const& name)
{
  pimpl_->Emit(name);
}

}
