// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (c) 2016 Canonical Ltd
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

#include <gtest/gtest.h>
using namespace testing;

#include "unity-shared/SystemdWrapper.h"

#include <UnityCore/GLibDBusServer.h>
#include <UnityCore/Variant.h>

#include "test_utils.h"

namespace
{

const std::string SYSTEMD =
R"(<node>
  <interface name="org.freedesktop.systemd1.Manager">
    <method name="StartUnit">
      <arg name="name" type="s" direction="in" />
      <arg name="mode" type="s" direction="in" />
      <arg name="job" type="o" direction="out" />
    </method>
    <method name="StopUnit">
      <arg name="name" type="s" direction="in" />
      <arg name="mode" type="s" direction="in" />
      <arg name="job" type="o" direction="out" />
    </method>
  </interface>
</node>)";

struct MockSystemdWrapper : unity::SystemdWrapper {
  MockSystemdWrapper()
    : SystemdWrapper(SystemdWrapper::TestMode())
  {}
};

struct TestSystemdWrapper : public Test
{
  TestSystemdWrapper()
  {
    systemd_server_ = std::make_shared<unity::glib::DBusServer>("com.canonical.Unity.Test.Systemd");
    systemd_server_->AddObjects(SYSTEMD, "/org/freedesktop/systemd1");

    Utils::WaitUntilMSec([this] { return systemd_server_->IsConnected(); });
  }

  unity::glib::DBusServer::Ptr systemd_server_;
  MockSystemdWrapper systemd_wrapper_;
};


TEST_F(TestSystemdWrapper, Start)
{
  bool start_sent = false;

  systemd_server_->GetObjects().front()->SetMethodsCallsHandler([&] (std::string const& method, GVariant* par) -> GVariant* {
    if (method == "StartUnit")
    {
      start_sent = true;

      std::string event_name = glib::Variant(g_variant_get_child_value(par, 0)).GetString();
      EXPECT_EQ("unity-screen-locked", event_name);
    }

    return nullptr;
  });

  systemd_wrapper_.Start("unity-screen-locked");
  Utils::WaitUntil(start_sent);
}

TEST_F(TestSystemdWrapper, Stop)
{
  bool stop_sent = false;

  systemd_server_->GetObjects().front()->SetMethodsCallsHandler([&] (std::string const& method, GVariant* par) -> GVariant* {
    if (method == "StopUnit")
    {
      stop_sent = true;

      std::string event_name = glib::Variant(g_variant_get_child_value(par, 0)).GetString();
      EXPECT_EQ("unity-screen-locked", event_name);
    }

    return nullptr;
  });

  systemd_wrapper_.Stop("unity-screen-locked");
  Utils::WaitUntil(stop_sent);
}

}
