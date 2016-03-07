// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2013,2015 Canonical Ltd
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
const std::string LOGIND_MANAGER_PATH = "/org/freedesktop/login1";
const std::string LOGIND_SESSION_PATH = "/org/freedesktop/login1/session/id0";
const std::string CONSOLE_KIT_PATH = "/org/freedesktop/ConsoleKit/Manager";
const std::string SESSION_MANAGER_PATH = "/org/gnome/SessionManager";
const std::string SESSION_MANAGER_PRESENCE_PATH = "/org/gnome/SessionManager/Presence";
const std::string DISPLAY_MANAGER_SEAT_PATH = "/org/freedesktop/DisplayManager/Seat0";

const std::string SESSION_OPTIONS = "com.canonical.indicator.session";
const std::string SUPPRESS_DIALOGS_KEY = "suppress-logout-restart-shutdown";

const std::string GNOME_LOCKDOWN_OPTIONS = "org.gnome.desktop.lockdown";
const std::string DISABLE_LOCKSCREEN_KEY = "disable-lock-screen";
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

const std::string LOGIND_MANAGER =
R"(<node>
  <interface name="org.freedesktop.login1.Manager">
    <method name="CanSuspend">
      <arg type="s" name="result" direction="out"/>
    </method>
    <method name="CanHibernate">
      <arg type="s" name="result" direction="out"/>
    </method>
    <method name="PowerOff">
      <arg type="b" name="interactive" direction="in"/>
    </method>
    <method name="Reboot">
      <arg type="b" name="interactive" direction="in"/>
    </method>
    <method name="Suspend">
      <arg type="b" name="interactive" direction="in"/>
    </method>
    <method name="Hibernate">
      <arg type="b" name="interactive" direction="in"/>
    </method>
    <method name="TerminateSession">
      <arg type="s" name="id" direction="in"/>
    </method>
  </interface>
</node>)";

const std::string LOGIND_SESSION =
R"(<node>
  <interface name="org.freedesktop.login1.Session">
    <signal name="Lock" />
    <signal name="Unlock" />
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

const std::string SESSION_MANAGER_PRESENCE =
R"(<node>
  <interface name="org.gnome.SessionManager.Presence">
    <signal name="StatusChanged">
      <arg name="new_status" type="u" />
    </signal>
  </interface>
</node>
)";

const std::string DISPLAY_MANAGER_SEAT =
R"(<node>
  <interface name="org.freedesktop.DisplayManager.Seat">
    <method name="SwitchToGreeter"/>
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
    upower_->GetObjects().front()->SetMethodsCallsHandler([&] (std::string const& method, GVariant*) -> GVariant* {
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

      return nullptr;
    });

    logind_ = std::make_shared<DBusServer>();
    logind_->AddObjects(introspection::LOGIND_MANAGER, LOGIND_MANAGER_PATH);
    logind_->AddObjects(introspection::LOGIND_SESSION, LOGIND_SESSION_PATH);
    logind_->GetObjects().front()->SetMethodsCallsHandler([&] (std::string const& method, GVariant*) -> GVariant* {
      if (method == "CanSuspend")
      {
        suspend_called = true;
        return g_variant_new("(s)", can_suspend_ ? "yes" : "no");
      }
      else if (method == "CanHibernate")
      {
        hibernate_called = true;
        return g_variant_new("(s)", can_hibernate_ ? "yes" : "no");
      }

      return nullptr;
    });

    console_kit_ = std::make_shared<DBusServer>();
    console_kit_->AddObjects(introspection::CONSOLE_KIT, CONSOLE_KIT_PATH);

    session_manager_ = std::make_shared<DBusServer>();
    session_manager_->AddObjects(introspection::SESSION_MANAGER, SESSION_MANAGER_PATH);
    session_manager_->AddObjects(introspection::SESSION_MANAGER_PRESENCE, SESSION_MANAGER_PRESENCE_PATH);
    session_manager_->GetObjects().front()->SetMethodsCallsHandler([&] (std::string const& method, GVariant*) -> GVariant* {
      if (method == "CanShutdown")
      {
        shutdown_called = true;
        return g_variant_new("(b)", can_shutdown_ ? TRUE : FALSE);
      }

      return nullptr;
    });

    display_manager_seat_ = std::make_shared<DBusServer>();
    display_manager_seat_->AddObjects(introspection::DISPLAY_MANAGER_SEAT, DISPLAY_MANAGER_SEAT_PATH);

    manager = std::make_shared<MockGnomeSessionManager>();
    shell_proxy_ = std::make_shared<DBusProxy>(TEST_SERVER_NAME, SHELL_OBJECT_PATH, SHELL_INTERFACE);

    // We need to wait until the session manager has setup its internal values.
    Utils::WaitUntilMSec(hibernate_called);
    Utils::WaitUntilMSec(suspend_called);
    Utils::WaitUntilMSec(shutdown_called);
    EXPECT_TRUE(hibernate_called);
    EXPECT_TRUE(suspend_called);
    EXPECT_TRUE(shutdown_called);
    Utils::WaitForTimeoutMSec(100);
  }

  void SetUp()
  {
    ASSERT_NE(manager, nullptr);
    Utils::WaitUntilMSec([] { return upower_->IsConnected(); });
    Utils::WaitUntilMSec([] { return logind_->IsConnected(); });
    Utils::WaitUntilMSec([] { return console_kit_->IsConnected(); });
    Utils::WaitUntilMSec([] { return session_manager_->IsConnected(); });
    Utils::WaitUntilMSec([] { return display_manager_seat_->IsConnected(); });
    Utils::WaitUntilMSec([] { return shell_proxy_->IsConnected();});
    ASSERT_TRUE(shell_proxy_->IsConnected());
    EnableInteractiveShutdown(true);

    // reset default logind methods, to avoid tests clobbering each other
    logind_->GetObjects().front()->SetMethodsCallsHandler([&] (std::string const& method, GVariant*) -> GVariant* {
      if (method == "CanSuspend")
        return g_variant_new("(s)", can_suspend_ ? "yes" : "no");
      else if (method == "CanHibernate")
        return g_variant_new("(s)", can_hibernate_ ? "yes" : "no");
      return nullptr;
    });
  }

  void TearDown()
  {
    manager->logout_requested.clear();
    manager->reboot_requested.clear();
    manager->shutdown_requested.clear();
    manager->cancel_requested.clear();
    shell_proxy_->DisconnectSignal();

    // By calling this we reset the internal pending action status
    bool cancelled = false;
    shell_proxy_->Connect("Canceled", [&cancelled] (GVariant*) { cancelled = true; });
    manager->CancelAction();
    Utils::WaitUntilMSec(cancelled);
    shell_proxy_->DisconnectSignal("Canceled");
  }

  static void TearDownTestCase()
  {
    bool cancelled = false;
    bool closed = false;
    shell_proxy_->Connect("Canceled", [&cancelled] (GVariant*) { cancelled = true; });
    shell_proxy_->Connect("Closed", [&closed] (GVariant*) { closed = true; });

    manager.reset();

    Utils::WaitUntilMSec(cancelled);
    Utils::WaitUntilMSec(closed);
    EXPECT_TRUE(cancelled);
    EXPECT_TRUE(closed);

    shell_proxy_.reset();
    upower_.reset();
    logind_.reset();
    console_kit_.reset();
    session_manager_.reset();
    display_manager_seat_.reset();
  }

  bool SettingsAvailable()
  {
    bool available = false;
    gchar** schemas = nullptr;

    g_settings_schema_source_list_schemas(g_settings_schema_source_get_default(), TRUE, &schemas, nullptr);

    for (unsigned i = 0; schemas[i]; ++i)
    {
      if (g_strcmp0(schemas[i], SESSION_OPTIONS.c_str()) == 0)
      {
        available = true;
        break;
      }
    }

    g_strfreev(schemas);

    return available;
  }

  void EnableInteractiveShutdown(bool enable)
  {
    ASSERT_TRUE(SettingsAvailable());
    glib::Object<GSettings> setting(g_settings_new(SESSION_OPTIONS.c_str()));
    g_settings_set_boolean(setting, SUPPRESS_DIALOGS_KEY.c_str(), enable ? FALSE : TRUE);
  }

  void DisableScreenLocking(bool disable)
  {
    glib::Object<GSettings> setting(g_settings_new(GNOME_LOCKDOWN_OPTIONS.c_str()));
    g_settings_set_boolean(setting, DISABLE_LOCKSCREEN_KEY.c_str(), disable ? TRUE : FALSE);
  }

  enum class Action : unsigned
  {
    LOGOUT = 0,
    SHUTDOWN,
    REBOOT
  };

  void ShellOpenAction(Action action)
  {
    shell_proxy_->Call("Open", g_variant_new("(uuuao)", action, 0, 0, nullptr));
  }

  void ShellOpenActionWithInhibitor(Action action)
  {
    GVariantBuilder builder;
    g_variant_builder_init(&builder, G_VARIANT_TYPE ("ao"));
    g_variant_builder_add(&builder, "o", "/any/inhibitor/object");
    shell_proxy_->Call("Open", g_variant_new("(uuuao)", action, 0, 0, &builder));
  }

  static bool can_shutdown_;
  static bool can_suspend_;
  static bool can_hibernate_;

  static session::Manager::Ptr manager;
  static DBusServer::Ptr upower_;
  static DBusServer::Ptr console_kit_;
  static DBusServer::Ptr logind_;
  static DBusServer::Ptr session_manager_;
  static DBusServer::Ptr display_manager_seat_;
  static DBusProxy::Ptr shell_proxy_;
};

session::Manager::Ptr TestGnomeSessionManager::manager;
DBusServer::Ptr TestGnomeSessionManager::upower_;
DBusServer::Ptr TestGnomeSessionManager::console_kit_;
DBusServer::Ptr TestGnomeSessionManager::logind_;
DBusServer::Ptr TestGnomeSessionManager::session_manager_;
DBusServer::Ptr TestGnomeSessionManager::display_manager_seat_;
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

TEST_F(TestGnomeSessionManager, HostName)
{
  EXPECT_EQ(manager->HostName(), g_get_host_name());
}

TEST_F(TestGnomeSessionManager, SwitchToGreeter)
{
  bool switch_called = false;

  display_manager_seat_->GetObjects().front()->SetMethodsCallsHandler([&] (std::string const& method, GVariant*) {
    if (method == "SwitchToGreeter")
      switch_called = true;

    return static_cast<GVariant*>(nullptr);
  });

  manager->SwitchToGreeter();

  Utils::WaitUntilMSec(switch_called);
  EXPECT_TRUE(switch_called);
}

TEST_F(TestGnomeSessionManager, ScreenSaverActivate)
{
  bool signal_emitted = false;
  bool signal_value = false;

  manager->screensaver_requested.connect([&signal_emitted, &signal_value] (bool value) {
    signal_emitted = true;
    signal_value = value;
  });

  manager->ScreenSaverActivate();

  Utils::WaitUntilMSec(signal_emitted);
  ASSERT_TRUE(signal_emitted);
  ASSERT_TRUE(signal_value);
}


TEST_F(TestGnomeSessionManager, ScreenSaverDeactivate)
{
  bool signal_emitted = false;
  bool signal_value = true;

  manager->screensaver_requested.connect([&signal_emitted, &signal_value] (bool value) {
    signal_emitted = true;
    signal_value = value;
  });

  manager->ScreenSaverDeactivate();

  Utils::WaitUntilMSec(signal_emitted);
  ASSERT_FALSE(signal_value);
}

TEST_F(TestGnomeSessionManager, LockScreen)
{
  bool lock_emitted = false;

  manager->lock_requested.connect([&lock_emitted]()
  {
    lock_emitted = true;
  });

  manager->LockScreen();

  Utils::WaitUntilMSec(lock_emitted);
  EXPECT_TRUE(lock_emitted);
}

TEST_F(TestGnomeSessionManager, PromptLockScreen)
{
  bool signal_emitted = false;

  manager->prompt_lock_requested.connect([&signal_emitted]() {
    signal_emitted = true;
  });

  manager->PromptLockScreen();

  Utils::WaitUntilMSec(signal_emitted);
  EXPECT_TRUE(signal_emitted);
}

TEST_F(TestGnomeSessionManager, Logout)
{
  bool logout_called = false;

  session_manager_->GetObjects().front()->SetMethodsCallsHandler([&] (std::string const& method, GVariant* par) -> GVariant* {
    if (method == "Logout")
    {
      logout_called = true;
      EXPECT_EQ(Variant(par).GetUInt32(), 1);
    }

    return nullptr;
  });

  manager->Logout();

  Utils::WaitUntilMSec(logout_called);
  EXPECT_TRUE(logout_called);
}

TEST_F(TestGnomeSessionManager, LogoutFallbackLogind)
{
  // This makes the standard call to return an error.
  session_manager_->GetObjects().front()->SetMethodsCallsHandler(nullptr);
  g_setenv("XDG_SESSION_ID", "logind-id0", TRUE);
  g_unsetenv("XDG_SESSION_COOKIE");

  bool logout_called = false;

  logind_->GetObjects().front()->SetMethodsCallsHandler([&] (std::string const& method, GVariant* par) -> GVariant* {
    if (method == "TerminateSession")
    {
      logout_called = true;
      EXPECT_EQ(Variant(par).GetString(), "logind-id0");
    }

    return nullptr;
  });

  manager->Logout();

  Utils::WaitUntilMSec(logout_called);
  EXPECT_TRUE(logout_called);
}

TEST_F(TestGnomeSessionManager, LogoutFallbackConsolekit)
{
  // This makes the standard call to return an error.
  session_manager_->GetObjects().front()->SetMethodsCallsHandler(nullptr);
  // disable logind
  logind_->GetObjects().front()->SetMethodsCallsHandler(nullptr);

  g_setenv("XDG_SESSION_COOKIE", "ck-session-cookie", TRUE);
  g_setenv("XDG_SESSION_ID", "logind-id0", TRUE);

  bool logout_called = false;

  console_kit_->GetObjects().front()->SetMethodsCallsHandler([&] (std::string const& method, GVariant* par) -> GVariant* {
    if (method == "CloseSession")
    {
      logout_called = true;
      EXPECT_EQ(Variant(par).GetString(), "ck-session-cookie");
    }

    return nullptr;
  });

  manager->Logout();

  Utils::WaitUntilMSec(logout_called);
  EXPECT_TRUE(logout_called);
}

TEST_F(TestGnomeSessionManager, LogoutFallbackConsolekitOnNoID)
{
  // This makes the standard call to return an error.
  session_manager_->GetObjects().front()->SetMethodsCallsHandler(nullptr);
  g_setenv("XDG_SESSION_COOKIE", "ck-session-cookie", TRUE);
  g_unsetenv("XDG_SESSION_ID");

  bool logout_called = false;

  console_kit_->GetObjects().front()->SetMethodsCallsHandler([&] (std::string const& method, GVariant* par) -> GVariant* {
    if (method == "CloseSession")
    {
      logout_called = true;
      EXPECT_EQ(Variant(par).GetString(), "ck-session-cookie");
    }

    return nullptr;
  });

  manager->Logout();

  Utils::WaitUntilMSec(logout_called);
  EXPECT_TRUE(logout_called);
}

TEST_F(TestGnomeSessionManager, Reboot)
{
  bool reboot_called = false;

  session_manager_->GetObjects().front()->SetMethodsCallsHandler([&] (std::string const& method, GVariant*) -> GVariant* {
    if (method == "Reboot")
      reboot_called = true;

    return nullptr;
  });

  manager->Reboot();

  Utils::WaitUntilMSec(reboot_called);
  EXPECT_TRUE(reboot_called);
}

TEST_F(TestGnomeSessionManager, RebootFallbackLogind)
{
  // This makes the standard call to return an error.
  session_manager_->GetObjects().front()->SetMethodsCallsHandler(nullptr);

  bool reboot_called = false;

  logind_->GetObjects().front()->SetMethodsCallsHandler([&] (std::string const& method, GVariant*) -> GVariant* {
    if (method == "Reboot")
      reboot_called = true;

    return nullptr;
  });

  manager->Reboot();

  Utils::WaitUntilMSec(reboot_called);
  EXPECT_TRUE(reboot_called);
}

TEST_F(TestGnomeSessionManager, RebootFallbackConsoleKit)
{
  // This makes the standard call to return an error.
  session_manager_->GetObjects().front()->SetMethodsCallsHandler(nullptr);
  // disable logind
  logind_->GetObjects().front()->SetMethodsCallsHandler(nullptr);

  bool reboot_called = false;

  console_kit_->GetObjects().front()->SetMethodsCallsHandler([&] (std::string const& method, GVariant*) -> GVariant* {
    if (method == "Restart")
      reboot_called = true;

    return nullptr;
  });

  manager->Reboot();

  Utils::WaitUntilMSec(reboot_called);
  EXPECT_TRUE(reboot_called);
}

TEST_F(TestGnomeSessionManager, Shutdown)
{
  bool shutdown_called = false;

  session_manager_->GetObjects().front()->SetMethodsCallsHandler([&] (std::string const& method, GVariant*) -> GVariant* {
    if (method == "Shutdown")
      shutdown_called = true;

    return nullptr;
  });

  manager->Shutdown();

  Utils::WaitUntilMSec(shutdown_called);
  EXPECT_TRUE(shutdown_called);
}

TEST_F(TestGnomeSessionManager, ShutdownFallbackLogind)
{
  // This makes the standard call to return an error.
  session_manager_->GetObjects().front()->SetMethodsCallsHandler(nullptr);

  bool shutdown_called = false;

  logind_->GetObjects().front()->SetMethodsCallsHandler([&] (std::string const& method, GVariant*) -> GVariant* {
    if (method == "PowerOff")
      shutdown_called = true;

    return nullptr;
  });

  manager->Shutdown();

  Utils::WaitUntilMSec(shutdown_called);
  EXPECT_TRUE(shutdown_called);
}

TEST_F(TestGnomeSessionManager, ShutdownFallbackConsoleKit)
{
  // This makes the standard call to return an error.
  session_manager_->GetObjects().front()->SetMethodsCallsHandler(nullptr);
  // disable logind
  logind_->GetObjects().front()->SetMethodsCallsHandler(nullptr);

  bool shutdown_called = false;

  console_kit_->GetObjects().front()->SetMethodsCallsHandler([&] (std::string const& method, GVariant*) -> GVariant* {
    if (method == "Stop")
      shutdown_called = true;

    return nullptr;
  });

  manager->Shutdown();

  Utils::WaitUntilMSec(shutdown_called);
  EXPECT_TRUE(shutdown_called);
}

TEST_F(TestGnomeSessionManager, SuspendUPower)
{
  bool suspend_called = false;

  // disable logind
  logind_->GetObjects().front()->SetMethodsCallsHandler(nullptr);

  upower_->GetObjects().front()->SetMethodsCallsHandler([&] (std::string const& method, GVariant*) -> GVariant* {
    if (method == "Suspend")
      suspend_called = true;

    return nullptr;
  });

  manager->Suspend();

  Utils::WaitUntilMSec(suspend_called);
  EXPECT_TRUE(suspend_called);
}

TEST_F(TestGnomeSessionManager, SuspendLogind)
{
  bool suspend_called = false;

  logind_->GetObjects().front()->SetMethodsCallsHandler([&] (std::string const& method, GVariant*) -> GVariant* {
    if (method == "Suspend")
      suspend_called = true;

    return nullptr;
  });

  manager->Suspend();

  Utils::WaitUntilMSec(suspend_called);
  EXPECT_TRUE(suspend_called);
}

TEST_F(TestGnomeSessionManager, HibernateUPower)
{
  bool hibernate_called = false;

  // disable logind
  logind_->GetObjects().front()->SetMethodsCallsHandler(nullptr);

  upower_->GetObjects().front()->SetMethodsCallsHandler([&] (std::string const& method, GVariant*) -> GVariant* {
    if (method == "Hibernate")
      hibernate_called = true;

    return nullptr;
  });

  manager->Hibernate();

  Utils::WaitUntilMSec(hibernate_called);
  EXPECT_TRUE(hibernate_called);
}

TEST_F(TestGnomeSessionManager, HibernateLogind)
{
  bool hibernate_called = false;

  logind_->GetObjects().front()->SetMethodsCallsHandler([&] (std::string const& method, GVariant*) -> GVariant* {
    if (method == "Hibernate")
      hibernate_called = true;

    return nullptr;
  });

  manager->Hibernate();

  Utils::WaitUntilMSec(hibernate_called);
  EXPECT_TRUE(hibernate_called);
}

TEST_F(TestGnomeSessionManager, CancelAction)
{
  bool cancelled = false;

  shell_proxy_->Connect("Canceled", [&cancelled] (GVariant*) { cancelled = true; });
  manager->CancelAction();

  Utils::WaitUntilMSec(cancelled);
  EXPECT_TRUE(cancelled);
}

TEST_F(TestGnomeSessionManager, LogoutRequested)
{
  bool logout_requested = false;
  bool cancelled = false;

  manager->logout_requested.connect([&logout_requested] (bool inhibitors) {
    logout_requested = true;
    EXPECT_FALSE(inhibitors);
  });

  shell_proxy_->Connect("Canceled", [&cancelled] (GVariant*) { cancelled = true; });

  ShellOpenAction(Action::LOGOUT);

  Utils::WaitUntilMSec(logout_requested);
  EXPECT_TRUE(logout_requested);

  Utils::WaitUntilMSec(cancelled);
  EXPECT_TRUE(cancelled);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"

struct InteractiveMode : TestGnomeSessionManager, testing::WithParamInterface<bool> {};
INSTANTIATE_TEST_CASE_P(TestGnomeSessionManager, InteractiveMode, testing::Bool());

TEST_P(/*TestGnomeSessionManager*/InteractiveMode, LogoutRequestedInhibitors)
{
  EnableInteractiveShutdown(GetParam());

  bool logout_requested = false;
  bool cancelled = false;

  manager->logout_requested.connect([&logout_requested] (bool inhibitors) {
    logout_requested = true;
    EXPECT_TRUE(inhibitors);
  });

  shell_proxy_->Connect("Canceled", [&cancelled] (GVariant*) { cancelled = true; });

  ShellOpenActionWithInhibitor(Action::LOGOUT);

  Utils::WaitUntilMSec(logout_requested);
  EXPECT_TRUE(logout_requested);

  Utils::WaitForTimeoutMSec(10);
  EXPECT_FALSE(cancelled);
}

TEST_F(TestGnomeSessionManager, ImmediateLogout)
{
  EnableInteractiveShutdown(false);
  bool logout_requested = false;
  bool confirmed = false;
  bool closed = false;

  manager->logout_requested.connect([&logout_requested] (bool inhibitors) {
    logout_requested = true;
    EXPECT_FALSE(inhibitors);
  });

  shell_proxy_->Connect("ConfirmedLogout", [&confirmed] (GVariant*) { confirmed = true; });
  shell_proxy_->Connect("Closed", [&closed] (GVariant*) { closed = true; });

  ShellOpenAction(Action::LOGOUT);

  Utils::WaitForTimeoutMSec(100);
  EXPECT_FALSE(logout_requested);

  Utils::WaitUntilMSec(confirmed);
  EXPECT_TRUE(confirmed);

  Utils::WaitUntilMSec(closed);
  EXPECT_TRUE(closed);
}

TEST_F(TestGnomeSessionManager, SimulateRealLogout)
{
  bool confirmed = false;
  bool closed = false;

  session_manager_->GetObjects().front()->SetMethodsCallsHandler([this] (std::string const& method, GVariant*) -> GVariant* {
    if (method == "Logout")
      ShellOpenAction(Action::LOGOUT);

    return nullptr;
  });

  manager->Logout();

  shell_proxy_->Connect("ConfirmedLogout", [&confirmed] (GVariant*) { confirmed = true; });
  shell_proxy_->Connect("Closed", [&closed] (GVariant*) { closed = true; });

  Utils::WaitUntilMSec(confirmed);
  EXPECT_TRUE(confirmed);

  Utils::WaitUntilMSec(closed);
  EXPECT_TRUE(closed);
}

TEST_F(TestGnomeSessionManager, ShutdownRequested)
{
  bool shutdown_requested = false;
  bool cancelled = false;

  manager->shutdown_requested.connect([&shutdown_requested] (bool inhibitors) {
    shutdown_requested = true;
    EXPECT_FALSE(inhibitors);
  });

  shell_proxy_->Connect("Canceled", [&cancelled] (GVariant*) { cancelled = true; });

  ShellOpenAction(Action::SHUTDOWN);

  Utils::WaitUntilMSec(shutdown_requested);
  EXPECT_TRUE(shutdown_requested);

  Utils::WaitUntilMSec(cancelled);
  EXPECT_TRUE(cancelled);
}

TEST_P(/*TestGnomeSessionManager*/InteractiveMode, ShutdownRequestedInhibitors)
{
  bool shutdown_requested = false;
  bool cancelled = false;

  manager->shutdown_requested.connect([&shutdown_requested] (bool inhibitors) {
    shutdown_requested = true;
    EXPECT_TRUE(inhibitors);
  });

  shell_proxy_->Connect("Canceled", [&cancelled] (GVariant*) { cancelled = true; });

  ShellOpenActionWithInhibitor(Action::SHUTDOWN);

  Utils::WaitUntilMSec(shutdown_requested);
  EXPECT_TRUE(shutdown_requested);

  Utils::WaitForTimeoutMSec(10);
  EXPECT_FALSE(cancelled);
}

TEST_F(TestGnomeSessionManager, ImmediateShutdown)
{
  EnableInteractiveShutdown(false);
  bool shutdown_requested = false;
  bool confirmed = false;
  bool closed = false;

  manager->shutdown_requested.connect([&shutdown_requested] (bool inhibitors) {
    shutdown_requested = true;
    EXPECT_FALSE(inhibitors);
  });

  shell_proxy_->Connect("ConfirmedShutdown", [&confirmed] (GVariant*) { confirmed = true; });
  shell_proxy_->Connect("Closed", [&closed] (GVariant*) { closed = true; });

  ShellOpenAction(Action::SHUTDOWN);

  Utils::WaitForTimeoutMSec(100);
  EXPECT_FALSE(shutdown_requested);

  Utils::WaitUntilMSec(confirmed);
  EXPECT_TRUE(confirmed);

  Utils::WaitUntilMSec(closed);
  EXPECT_TRUE(closed);
}

TEST_F(TestGnomeSessionManager, SimulateRealShutdown)
{
  bool confirmed = false;
  bool closed = false;

  session_manager_->GetObjects().front()->SetMethodsCallsHandler([this] (std::string const& method, GVariant*) -> GVariant* {
    if (method == "Shutdown")
      ShellOpenAction(Action::SHUTDOWN);

    return nullptr;
  });

  manager->Shutdown();

  shell_proxy_->Connect("ConfirmedShutdown", [&confirmed] (GVariant*) { confirmed = true; });
  shell_proxy_->Connect("Closed", [&closed] (GVariant*) { closed = true; });

  Utils::WaitUntilMSec(confirmed);
  EXPECT_TRUE(confirmed);

  Utils::WaitUntilMSec(closed);
  EXPECT_TRUE(closed);
}

TEST_F(TestGnomeSessionManager, RebootRequested)
{
  bool reboot_requested = false;
  bool cancelled = false;

  manager->reboot_requested.connect([&reboot_requested] (bool inhibitors) {
    reboot_requested = true;
    EXPECT_FALSE(inhibitors);
  });

  shell_proxy_->Connect("Canceled", [&cancelled] (GVariant*) { cancelled = true; });

  ShellOpenAction(Action::REBOOT);

  Utils::WaitUntilMSec(reboot_requested);
  EXPECT_TRUE(reboot_requested);

  Utils::WaitUntilMSec(cancelled);
  EXPECT_TRUE(cancelled);
}

TEST_P(/*TestGnomeSessionManager*/InteractiveMode, RebootRequestedInhibitors)
{
  bool reboot_requested = false;
  bool cancelled = false;

  manager->reboot_requested.connect([&reboot_requested] (bool inhibitors) {
    reboot_requested = true;
    EXPECT_TRUE(inhibitors);
  });

  shell_proxy_->Connect("Canceled", [&cancelled] (GVariant*) { cancelled = true; });

  ShellOpenActionWithInhibitor(Action::REBOOT);

  Utils::WaitUntilMSec(reboot_requested);
  EXPECT_TRUE(reboot_requested);

  Utils::WaitForTimeoutMSec(10);
  EXPECT_FALSE(cancelled);
}

#pragma GCC diagnostic pop

TEST_F(TestGnomeSessionManager, ImmediateReboot)
{
  EnableInteractiveShutdown(false);
  bool reboot_requested = false;
  bool confirmed = false;
  bool closed = false;

  manager->reboot_requested.connect([&reboot_requested] (bool inhibitors) {
    reboot_requested = true;
    EXPECT_FALSE(inhibitors);
  });

  shell_proxy_->Connect("ConfirmedReboot", [&confirmed] (GVariant*) { confirmed = true; });
  shell_proxy_->Connect("Closed", [&closed] (GVariant*) { closed = true; });

  ShellOpenAction(Action::REBOOT);

  Utils::WaitForTimeoutMSec(100);
  EXPECT_FALSE(reboot_requested);

  Utils::WaitUntilMSec(confirmed);
  EXPECT_TRUE(confirmed);

  Utils::WaitUntilMSec(closed);
  EXPECT_TRUE(closed);
}

TEST_F(TestGnomeSessionManager, SimulateRealReboot)
{
  bool confirmed = false;
  bool closed = false;

  session_manager_->GetObjects().front()->SetMethodsCallsHandler([this] (std::string const& method, GVariant*) -> GVariant* {
    if (method == "Reboot")
      ShellOpenAction(Action::REBOOT);

    return nullptr;
  });

  manager->Reboot();

  shell_proxy_->Connect("ConfirmedReboot", [&confirmed] (GVariant*) { confirmed = true; });
  shell_proxy_->Connect("Closed", [&closed] (GVariant*) { closed = true; });

  Utils::WaitUntilMSec(confirmed);
  EXPECT_TRUE(confirmed);

  Utils::WaitUntilMSec(closed);
  EXPECT_TRUE(closed);
}

TEST_F(TestGnomeSessionManager, CancelRequested)
{
  bool cancel_requested = false;
  bool closed = false;

  manager->cancel_requested.connect([&cancel_requested] { cancel_requested = true; });
  shell_proxy_->Connect("Closed", [&closed] (GVariant*) { closed = true; });

  shell_proxy_->Call("Close");

  Utils::WaitUntilMSec(cancel_requested);
  EXPECT_TRUE(cancel_requested);

  Utils::WaitUntilMSec(closed);
  EXPECT_TRUE(closed);
}

TEST_F(TestGnomeSessionManager, LogindLock)
{
  bool signal_emitted = false;

  manager->prompt_lock_requested.connect([&signal_emitted]()
  {
    signal_emitted = true;
  });

  logind_->GetObject("org.freedesktop.login1.Session")->EmitSignal("Lock");

  Utils::WaitUntilMSec(signal_emitted);
  EXPECT_TRUE(signal_emitted);
}

TEST_F(TestGnomeSessionManager, LogindUnLock)
{
  bool unlock_emitted = false;

  manager->unlock_requested.connect([&unlock_emitted]()
  {
    unlock_emitted = true;
  });

  logind_->GetObject("org.freedesktop.login1.Session")->EmitSignal("Unlock");

  Utils::WaitUntilMSec(unlock_emitted);
  EXPECT_TRUE(unlock_emitted);
}

TEST_F(TestGnomeSessionManager, UNSTABLE_TEST(NoLockWhenLockingDisabled))
{
  bool lock_emitted = false;
  bool screensaver_emitted = false;
  bool screensaver_value = false;

  manager->lock_requested.connect([&lock_emitted]() {
    lock_emitted = true;
  });

  manager->screensaver_requested.connect([&screensaver_emitted, &screensaver_value] (bool value) {
    screensaver_emitted = true;
    screensaver_value = value;
  });

  DisableScreenLocking(true);
  manager->LockScreen();

  Utils::WaitUntilMSec(screensaver_emitted);
  EXPECT_TRUE(screensaver_value);
  EXPECT_FALSE(lock_emitted);

  DisableScreenLocking(false);
}

TEST_F(TestGnomeSessionManager, PresenceStatusChanged)
{
  bool signal_emitted = false;
  bool idle = false;

  manager->presence_status_changed.connect([&signal_emitted, &idle] (bool is_idle) {
    signal_emitted = true;
    idle = is_idle;
  });

  session_manager_->GetObject("org.gnome.SessionManager.Presence")->EmitSignal("StatusChanged", g_variant_new("(u)", 3));

  Utils::WaitUntilMSec(signal_emitted);
  ASSERT_TRUE(signal_emitted);
  ASSERT_TRUE(idle);

  signal_emitted = false;

  session_manager_->GetObject("org.gnome.SessionManager.Presence")->EmitSignal("StatusChanged", g_variant_new("(u)", 0));

  Utils::WaitUntilMSec(signal_emitted);
  ASSERT_TRUE(signal_emitted);
  ASSERT_FALSE(idle);
}

} // Namespace
