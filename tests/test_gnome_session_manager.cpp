// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2013 Canonical Ltd
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
 * Authored by: Marco Trevisan (Treviño) <marco.trevisan@canonical.com>
 */

#include <gmock/gmock.h>
#include <UnityCore/GLibDBusProxy.h>
#include <UnityCore/GLibDBusServer.h>
#include <UnityCore/GnomeSessionManager.h>
#include "test_utils.h"

using namespace unity;
using namespace unity::glib;

namespace
{
namespace
{
const std::string TEST_SERVER_NAME = "com.canonical.Unity.Test.GnomeManager";

const std::string SHELL_INTERFACE = "org.gnome.SessionManager.EndSessionDialog";
const std::string SHELL_OBJECT_PATH = "/org/gnome/SessionManager/EndSessionDialog";

const std::string UPOWER_PATH = "/org/freedesktop/UPower";
const std::string SCREEN_SAVER_PATH = "/org/gnome/ScreenSaver";
const std::string SESSION_MANAGER_PATH = "/org/gnome/SessionManager";
}

namespace introspection
{
const std::string UPOWER =
R"(<node>
  <interface name="org.freedesktop.UPower">
    <method name="Suspend"/>
    <method name="Hibernate"/>
    <method name="HibernateAllowed">
      <arg type="b" name="hibernate_allowed" direction="out"/>
    </method>
    <method name="SuspendAllowed">
      <arg type="b" name="suspend_allowed" direction="out"/>
    </method>
  </interface>
</node>)";

const std::string SCREEN_SAVER =
R"(<node>
  <interface name="org.gnome.ScreenSaver">
    <method name="Lock"/>
    <method name="SimulateUserActivity"/>
  </interface>
</node>)";

const std::string SESSION_MANAGER =
R"(<node>
  <interface name="org.gnome.SessionManager">
    <method name="Logout">
      <arg type="u" name="type" direction="in"/>
    </method>
    <method name="Reboot"/>
    <signal name="Shutdown"/>
    <method name="CanShutdown">
      <arg type="b" name="can_shutdown" direction="out"/>
    </method>
  </interface>
</node>
)";
}

struct MockGnomeSessionManager : session::GnomeManager {
  MockGnomeSessionManager()
    : GnomeManager(GnomeManager::TestMode())
  {}
};

struct TestGnomeSessionManager : testing::Test
{
  TestGnomeSessionManager()
  {}

  static void SetUpTestCase()
  {
    can_shutdown_ = (g_random_int() % 2) ? true : false;
    can_suspend_ = (g_random_int() % 2) ? true : false;
    can_hibernate_ = (g_random_int() % 2) ? true : false;

    upower_ = std::make_shared<DBusServer>();
    upower_->AddObjects(introspection::UPOWER, UPOWER_PATH);
    upower_->GetObjects().front()->SetMethodsCallsHandler([] (std::string const& method, GVariant*) {
      if (method == "SuspendAllowed")
        return g_variant_new("(b)", can_suspend_ ? TRUE : FALSE);
      else if (method == "HibernateAllowed")
        return g_variant_new("(b)", can_hibernate_ ? TRUE : FALSE);

      return static_cast<GVariant*>(nullptr);
    });

    screen_saver_ = std::make_shared<DBusServer>();
    screen_saver_->AddObjects(introspection::SCREEN_SAVER, SCREEN_SAVER_PATH);

    session_manager_ = std::make_shared<DBusServer>();
    session_manager_->AddObjects(introspection::SESSION_MANAGER, SESSION_MANAGER_PATH);
    session_manager_->GetObjects().front()->SetMethodsCallsHandler([] (std::string const& method, GVariant*) {
      if (method == "CanShutdown")
        return g_variant_new("(b)", can_shutdown_ ? TRUE : FALSE);

      return static_cast<GVariant*>(nullptr);
    });

    manager = std::make_shared<MockGnomeSessionManager>();

    shell_proxy_ = std::make_shared<DBusProxy>(TEST_SERVER_NAME, SHELL_OBJECT_PATH, SHELL_INTERFACE);

    Utils::WaitForTimeout(2);
  }

  void SetUp()
  {
    Utils::WaitUntilMSec([this] { return upower_->IsConnected(); });
    Utils::WaitUntilMSec([this] { return screen_saver_->IsConnected(); });
    Utils::WaitUntilMSec([this] { return session_manager_->IsConnected(); });
    Utils::WaitUntil([this] { return shell_proxy_->IsConnected();}, true, 3);
    ASSERT_TRUE(shell_proxy_->IsConnected());
  }

  static bool can_shutdown_;
  static bool can_suspend_;
  static bool can_hibernate_;

  static session::Manager::Ptr manager;
  static DBusServer::Ptr upower_;
  static DBusServer::Ptr screen_saver_;
  static DBusServer::Ptr session_manager_;
  static DBusProxy::Ptr shell_proxy_;
};

session::Manager::Ptr TestGnomeSessionManager::manager;
DBusServer::Ptr TestGnomeSessionManager::upower_;
DBusServer::Ptr TestGnomeSessionManager::screen_saver_;
DBusServer::Ptr TestGnomeSessionManager::session_manager_;
DBusProxy::Ptr TestGnomeSessionManager::shell_proxy_;
bool TestGnomeSessionManager::can_shutdown_;
bool TestGnomeSessionManager::can_suspend_;
bool TestGnomeSessionManager::can_hibernate_;

TEST_F(TestGnomeSessionManager, CanShutdown)
{
  EXPECT_EQ(manager->CanShutdown(), can_shutdown_);
}

TEST_F(TestGnomeSessionManager, CanHibernate)
{
  EXPECT_EQ(manager->CanHibernate(), can_hibernate_);
}

TEST_F(TestGnomeSessionManager, CanSuspend)
{
  EXPECT_EQ(manager->CanSuspend(), can_suspend_);
}

} // Namespace
