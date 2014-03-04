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

#include <UnityCore/GLibDBusServer.h>
#include <UnityCore/Variant.h>

#include "test_utils.h"

namespace
{

const std::string UPSTART =
R"(<node>
  <interface name="com.ubuntu.Upstart0_6">
    <method name="EmitEvent">
      <arg name="name" type="s" direction="in" />
      <arg name="env" type="as" direction="in" />
      <arg name="wait" type="b" direction="in" />
    </method>

    <signal name="EventEmitted">
      <arg name="name" type="s" />
      <arg name="env" type="as" />
    </signal>
  </interface>
</node>)";

struct MockUpstartWrapper : unity::UpstartWrapper {
  MockUpstartWrapper()
    : UpstartWrapper(UpstartWrapper::TestMode())
  {}
};

struct TestUpstartWrapper : public Test
{
  TestUpstartWrapper()
  {
    upstart_server_ = std::make_shared<unity::glib::DBusServer>("com.canonical.Unity.Test.Upstart");
    upstart_server_->AddObjects(UPSTART, "/com/ubuntu/Upstart");

    Utils::WaitUntilMSec([this] { return upstart_server_->IsConnected(); });
  }

  unity::glib::DBusServer::Ptr upstart_server_;
  MockUpstartWrapper upstart_wrapper_;
};


TEST_F(TestUpstartWrapper, Emit)
{
  bool event_emitted = false;

  upstart_server_->GetObjects().front()->SetMethodsCallsHandler([&] (std::string const& method, GVariant* par) -> GVariant* {
    if (method == "EmitEvent")
    {
      event_emitted = true;

      std::string event_name = glib::Variant(g_variant_get_child_value(par, 0)).GetString();
      EXPECT_EQ("desktop-lock", event_name);
    }

    return nullptr;
  });

  upstart_wrapper_.Emit("desktop-lock");
  Utils::WaitUntil(event_emitted);
}

}
