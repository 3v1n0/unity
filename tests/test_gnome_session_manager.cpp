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
#include <UnityCore/Variant.h>
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
const std::string CONSOLE_KIT_PATH = "/org/freedesktop/ConsoleKit/Manager";
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

const std::string CONSOLE_KIT =
R"(<node>
  <interface name="org.freedesktop.ConsoleKit.Manager">
    <method name="Stop"/>
    <method name="Restart"/>
    <method name="CloseSession">
      <arg type="s" name="cookie" direction="in"/>
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
    <method name="Shutdown"/>
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
  static void SetUpTestCase()
  {
    can_shutdown_ = (g_random_int() % 2) ? true : false;
    can_suspend_ = (g_random_int() % 2) ? true : false;
    can_hibernate_ = (g_random_int() % 2) ? true : false;

    bool shutdown_called = false;
    bool hibernate_called = false;
    bool suspend_called = false;

    upower_ = std::make_shared<DBusServer>();
    upower_->AddObjects(introspection::UPOWER, UPOWER_PATH);
    upower_->GetObjects().front()->SetMethodsCallsHandler([&] (std::string const& method, GVariant*) {
      if (method == "SuspendAllowed")
      {
        suspend_called = true;
        return g_variant_new("(b)", can_suspend_ ? TRUE : FALSE);
      }
      else if (method == "HibernateAllowed")
      {
        hibernate_called = true;
        return g_variant_new("(b)", can_hibernate_ ? TRUE : FALSE);
      }

      return static_cast<GVariant*>(nullptr);
    });

    console_kit_ = std::make_shared<DBusServer>();
    console_kit_->AddObjects(introspection::CONSOLE_KIT, CONSOLE_KIT_PATH);

    screen_saver_ = std::make_shared<DBusServer>();
    screen_saver_->AddObjects(introspection::SCREEN_SAVER, SCREEN_SAVER_PATH);

    session_manager_ = std::make_shared<DBusServer>();
    session_manager_->AddObjects(introspection::SESSION_MANAGER, SESSION_MANAGER_PATH);
    session_manager_->GetObjects().front()->SetMethodsCallsHandler([&] (std::string const& method, GVariant*) {
      if (method == "CanShutdown")
      {
        shutdown_called = true;
        return g_variant_new("(b)", can_shutdown_ ? TRUE : FALSE);
      }

      return static_cast<GVariant*>(nullptr);
    });

    manager = std::make_shared<MockGnomeSessionManager>();

    shell_proxy_ = std::make_shared<DBusProxy>(TEST_SERVER_NAME, SHELL_OBJECT_PATH, SHELL_INTERFACE);

    // We need to wait until the session manager has got its capabilities.
    Utils::WaitUntil(hibernate_called, 3);
    Utils::WaitUntil(suspend_called, 3);
    Utils::WaitUntil(shutdown_called, 3);
    Utils::WaitForTimeoutMSec(100);
  }

  void SetUp()
  {
    ASSERT_NE(manager, nullptr);
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
  static DBusServer::Ptr console_kit_;
  static DBusServer::Ptr screen_saver_;
  static DBusServer::Ptr session_manager_;
  static DBusProxy::Ptr shell_proxy_;
};

session::Manager::Ptr TestGnomeSessionManager::manager;
DBusServer::Ptr TestGnomeSessionManager::upower_;
DBusServer::Ptr TestGnomeSessionManager::console_kit_;
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

TEST_F(TestGnomeSessionManager, RealName)
{
  const char* name = g_get_real_name();

  if (!name || g_strcmp0(name, "Unknown") == 0)
    EXPECT_TRUE(manager->RealName().empty());
  else
    EXPECT_EQ(manager->RealName(), name);
}

TEST_F(TestGnomeSessionManager, UserName)
{
  EXPECT_EQ(manager->UserName(), g_get_user_name());
}

TEST_F(TestGnomeSessionManager, LockScreen)
{
  bool lock_called = false;
  bool simulate_activity_called = false;

  screen_saver_->GetObjects().front()->SetMethodsCallsHandler([&] (std::string const& method, GVariant*) {
    if (method == "Lock")
    {
      lock_called = true;
      EXPECT_FALSE(simulate_activity_called);
    }
    else if (method == "SimulateUserActivity")
    {
      simulate_activity_called = true;
      EXPECT_TRUE(lock_called);
    }

    return static_cast<GVariant*>(nullptr);
  });

  manager->LockScreen();

  Utils::WaitUntil(lock_called, 2);
  EXPECT_TRUE(lock_called);

  Utils::WaitUntil(simulate_activity_called, 2);
  EXPECT_TRUE(simulate_activity_called);
}

TEST_F(TestGnomeSessionManager, Logout)
{
  bool logout_called = false;

  session_manager_->GetObjects().front()->SetMethodsCallsHandler([&] (std::string const& method, GVariant* par) {
    if (method == "Logout")
    {
      logout_called = true;
      EXPECT_EQ(Variant(par).GetUInt(), 1);
    }

    return static_cast<GVariant*>(nullptr);
  });

  manager->Logout();

  Utils::WaitUntil(logout_called, 2);
  EXPECT_TRUE(logout_called);
}

TEST_F(TestGnomeSessionManager, LogoutFallback)
{
  // This makes the standard call to return an error.
  session_manager_->GetObjects().front()->SetMethodsCallsHandler(nullptr);
  const gchar* cookie = g_getenv("XDG_SESSION_COOKIE");

  if (!cookie)
    g_setenv("XDG_SESSION_COOKIE", "foo-cookie", TRUE);

  bool logout_called = false;

  console_kit_->GetObjects().front()->SetMethodsCallsHandler([&] (std::string const& method, GVariant* par) {
    if (method == "CloseSession")
    {
      logout_called = true;
      EXPECT_EQ(Variant(par).GetString(), cookie);
    }

    return static_cast<GVariant*>(nullptr);
  });

  manager->Logout();

  Utils::WaitUntil(logout_called, 2);
  EXPECT_TRUE(logout_called);
}

TEST_F(TestGnomeSessionManager, Reboot)
{
  bool reboot_called = false;

  session_manager_->GetObjects().front()->SetMethodsCallsHandler([&] (std::string const& method, GVariant*) {
    if (method == "Reboot")
      reboot_called = true;

    return static_cast<GVariant*>(nullptr);
  });

  manager->Reboot();

  Utils::WaitUntil(reboot_called, 2);
  EXPECT_TRUE(reboot_called);
}

TEST_F(TestGnomeSessionManager, RebootFallback)
{
  // This makes the standard call to return an error.
  session_manager_->GetObjects().front()->SetMethodsCallsHandler(nullptr);

  bool reboot_called = false;

  console_kit_->GetObjects().front()->SetMethodsCallsHandler([&] (std::string const& method, GVariant*) {
    if (method == "Restart")
      reboot_called = true;

    return static_cast<GVariant*>(nullptr);
  });

  manager->Reboot();

  Utils::WaitUntil(reboot_called, 2);
  EXPECT_TRUE(reboot_called);
}

TEST_F(TestGnomeSessionManager, Shutdown)
{
  bool shutdown_called = false;

  session_manager_->GetObjects().front()->SetMethodsCallsHandler([&] (std::string const& method, GVariant*) {
    if (method == "Shutdown")
      shutdown_called = true;

    return static_cast<GVariant*>(nullptr);
  });

  manager->Shutdown();

  Utils::WaitUntil(shutdown_called, 2);
  EXPECT_TRUE(shutdown_called);
}

TEST_F(TestGnomeSessionManager, ShutdownFallback)
{
  // This makes the standard call to return an error.
  session_manager_->GetObjects().front()->SetMethodsCallsHandler(nullptr);

  bool shutdown_called = false;

  console_kit_->GetObjects().front()->SetMethodsCallsHandler([&] (std::string const& method, GVariant*) {
    if (method == "Stop")
      shutdown_called = true;

    return static_cast<GVariant*>(nullptr);
  });

  manager->Shutdown();

  Utils::WaitUntil(shutdown_called, 2);
  EXPECT_TRUE(shutdown_called);
}

} // Namespace
