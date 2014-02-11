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

#include <gtest/gtest.h>
using namespace testing;

#include "unity-shared/UpstartWrapper.h"

#include <UnityCore/GLibDBusProxy.h>

#include "test_utils.h"

namespace
{

struct TestUpstartWrapper : public Test
{
  TestUpstartWrapper()
  {
    upstart_proxy_ = std::make_shared<unity::glib::DBusProxy>("com.ubuntu.Upstart",
                                                              "/com/ubuntu/Upstart", 
                                                              "com.ubuntu.Upstart0_6");
  }

  unity::UpstartWrapper upstart_wrapper_;
  unity::glib::DBusProxy::Ptr upstart_proxy_;
};

TEST_F(TestUpstartWrapper, Constuction)
{
  Utils::WaitUntilMSec([this] {
  	return upstart_proxy_->IsConnected();
  });
}

TEST_F(TestUpstartWrapper, Emit)
{
  bool event_emitted = false;

  upstart_proxy_->Connect("EventEmitted", [&event_emitted] (GVariant* value) {
    std::string event_name = glib::Variant(g_variant_get_child_value(value, 0)).GetString();
    event_emitted = (event_name == "desktop-lock");
  });

  upstart_wrapper_.Emit("desktop-lock");

  Utils::WaitUntil(event_emitted, 5);
}

}
