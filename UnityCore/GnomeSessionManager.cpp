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
* Authored by: Marco Trevisan (Treviño) <marco@ubuntu.com>
*/

#include "GnomeSessionManagerImpl.h"

#include <NuxCore/Logger.h>
#include "Variant.h"

namespace unity
{
namespace session
{
DECLARE_LOGGER(logger, "unity.session.gnome");

// Private implementation
namespace shell
{
const std::string DBUS_NAME = "org.gnome.Shell";
const std::string DBUS_INTERFACE = "org.gnome.SessionManager.EndSessionDialog";
const std::string DBUS_OBJECT_PATH = "/org/gnome/SessionManager/EndSessionDialog";
const std::string INTROSPECTION_XML =
R"(<node>
  <interface name="org.gnome.SessionManager.EndSessionDialog">
    <method name="Open">
      <arg type="u" name="type" direction="in">
      </arg>
      <arg type="u" name="arg_1" direction="in">
      </arg>
      <arg type="u" name="max_wait" direction="in">
      </arg>
      <arg type="ao" name="inhibitors" direction="in">
      </arg>
    </method>
    <method name="Close">
    </method>
    <signal name="ConfirmedLogout">
    </signal>
    <signal name="ConfirmedReboot">
    </signal>
    <signal name="ConfirmedShutdown">
    </signal>
    <signal name="Canceled">
    </signal>
    <signal name="Closed">
    </signal>
  </interface>
</node>)";
}

namespace testing
{
const std::string DBUS_NAME = "com.canonical.Unity.Test.GnomeManager";
}

namespace
{
const std::string SESSION_OPTIONS = "com.canonical.indicator.session";
const std::string SUPPRESS_DIALOGS_KEY = "suppress-logout-restart-shutdown";
}

GnomeManager::Impl::Impl(GnomeManager* manager, bool test_mode)
  : manager_(manager)
  , test_mode_(test_mode)
  , can_shutdown_(true)
  , can_suspend_(false)
  , can_hibernate_(false)
  , pending_action_(shell::Action::NONE)
  , shell_server_(test_mode_ ? testing::DBUS_NAME : shell::DBUS_NAME)
{
  shell_server_.AddObjects(shell::INTROSPECTION_XML, shell::DBUS_OBJECT_PATH);
  shell_object_ = shell_server_.GetObject(shell::DBUS_INTERFACE);
  shell_object_->SetMethodsCallsHandler(sigc::mem_fun(this, &Impl::OnShellMethodCall));

  // TODO (andy) fallback to CK. But we need to figure out how to do that.
  {
    const char* session_id = g_getenv("XDG_SESSION_ID");
    session_id = session_id ? session_id : "";

    login_proxy_ = std::make_shared<glib::DBusProxy>(test_mode_ ? testing::DBUS_NAME : "org.freedesktop.login1",
                                                     std::string("/org/freedesktop/login1/session/") + session_id,
                                                     "org.freedesktop.login1.Session",
                                                     test_mode_ ? G_BUS_TYPE_SESSION : G_BUS_TYPE_SYSTEM);

    login_proxy_->Connect("Lock", [this](GVariant*){
      manager_->lock_requested.emit();
    });

    login_proxy_->Connect("Unlock", [this](GVariant*){
      manager_->unlock_requested.emit();
    });
  }

  CallLogindMethod("CanHibernate", nullptr, [this] (GVariant* variant, glib::Error const& err) {
    if (err)
    {
      // fall back to upower
      CallUPowerMethod("HibernateAllowed", [this] (GVariant* variant) {
        can_hibernate_ = glib::Variant(variant).GetBool();
        LOG_INFO(logger) << "Can hibernate (upower): " << can_hibernate_;
      });
    }
    else
    {
      can_hibernate_ = glib::Variant(variant).GetString() == "yes";
      LOG_INFO(logger) << "Can hibernate (logind): " << can_hibernate_;
    }
  });

  CallLogindMethod("CanSuspend", nullptr, [this] (GVariant* variant, glib::Error const& err) {
    if (err)
    {
      // fall back to upower
      CallUPowerMethod("SuspendAllowed", [this] (GVariant* variant) {
        can_suspend_ = glib::Variant(variant).GetBool();
        LOG_INFO(logger) << "Can suspend (upower): " << can_suspend_;
      });
    }
    else
    {
      can_suspend_ = glib::Variant(variant).GetString() == "yes";
      LOG_INFO(logger) << "Can suspend (logind): " << can_suspend_;
    }
  });

  CallGnomeSessionMethod("CanShutdown", nullptr, [this] (GVariant* variant, glib::Error const& e) {
    if (!e)
    {
      can_shutdown_ = glib::Variant(variant).GetBool();
      LOG_INFO(logger) << "Can shutdown: " << can_shutdown_;
    }
  });
}

GnomeManager::Impl::~Impl()
{
  CancelAction();
  ClosedDialog();
}

bool GnomeManager::Impl::InteractiveMode()
{
  bool schema_found = false;
  gchar** schemas = nullptr;

  g_settings_schema_source_list_schemas(g_settings_schema_source_get_default(), TRUE, &schemas, nullptr);

  for (unsigned i = 0; schemas[i]; ++i)
  {
    if (g_strcmp0(schemas[i], SESSION_OPTIONS.c_str()) == 0)
    {
      schema_found = true;
      break;
    }
  }

  g_strfreev(schemas);

  if (!schema_found)
    return true;

  glib::Object<GSettings> setting(g_settings_new(SESSION_OPTIONS.c_str()));
  return g_settings_get_boolean(setting, SUPPRESS_DIALOGS_KEY.c_str()) != TRUE;
}

GVariant* GnomeManager::Impl::OnShellMethodCall(std::string const& method, GVariant* parameters)
{
  LOG_DEBUG(logger) << "Called method '" << method << "'";

  if (method == "Open")
  {
    // This method is called both when the session manager was invoked from
    // any external caller (such as the gnome-session) and when we call the
    // gnome-session methods internally (in the GnomeManager).
    // So, in the first case we just ignore the request informing our clients,
    // while in the second case, we know what we requested and we can immediately
    // proceed with the requested action.

    shell::Action action;
    unsigned arg1, timeout_length;
    GVariantIter *inhibitors;
    g_variant_get(parameters, "(uuuao)", &action, &arg1, &timeout_length, &inhibitors);
    bool has_inibitors = (g_variant_iter_n_children(inhibitors) > 0);
    g_variant_iter_free(inhibitors);

    LOG_INFO(logger) << "Got Open request for action " << unsigned(action)
                     << " with inhibitors " << has_inibitors;

    if (pending_action_ == shell::Action::NONE)
    {
      if (!InteractiveMode() && !has_inibitors)
      {
        // If we're in non-interactive mode and we don't have inhibitors,
        // We must immediately proceed with the requested action.
        LOG_INFO(logger) << "Not using interactive mode, proceeding with requested action...";
        pending_action_ = action;
      }
    }

    if (pending_action_ == shell::Action::NONE)
    {
      if (has_inibitors)
      {
        // If there are inhibitors we need to use the "standard" way
        // otherwise the action will be postponed until an inhibitor is closed
        pending_action_ = action;
      }
      else
      {
        CancelAction();
      }

      switch (action)
      {
        case shell::Action::LOGOUT:
          manager_->logout_requested.emit(has_inibitors);
          break;
        case shell::Action::SHUTDOWN:
          manager_->shutdown_requested.emit(has_inibitors);
          break;
        case shell::Action::REBOOT:
          manager_->reboot_requested.emit(has_inibitors);
          break;
        default:
          break;
      }
    }
    else if (pending_action_ == action)
    {
      switch (action)
      {
        case shell::Action::LOGOUT:
          ConfirmLogout();
          ClosedDialog();
          break;
        case shell::Action::SHUTDOWN:
          ConfirmShutdown();
          ClosedDialog();
          break;
        case shell::Action::REBOOT:
          ConfirmReboot();
          ClosedDialog();
          break;
        default:
          break;
      }
    }
  }
  else if (method == "Close")
  {
    manager_->cancel_requested.emit();
    ClosedDialog();
  }

  return nullptr;
}

void GnomeManager::Impl::ConfirmLogout()
{
  pending_action_ = shell::Action::NONE;
  shell_object_->EmitSignal("ConfirmedLogout");
}

void GnomeManager::Impl::ConfirmReboot()
{
  pending_action_ = shell::Action::NONE;
  shell_object_->EmitSignal("ConfirmedReboot");
}

void GnomeManager::Impl::ConfirmShutdown()
{
  pending_action_ = shell::Action::NONE;
  shell_object_->EmitSignal("ConfirmedShutdown");
}

void GnomeManager::Impl::CancelAction()
{
  pending_action_ = shell::Action::NONE;
  shell_object_->EmitSignal("Canceled");
}

void GnomeManager::Impl::ClosedDialog()
{
  shell_object_->EmitSignal("Closed");
}

void GnomeManager::Impl::EnsureCancelPendingAction()
{
  if (pending_action_ == shell::Action::NONE)
    return;

  CancelAction();
}

void GnomeManager::Impl::CallGnomeSessionMethod(std::string const& method, GVariant* parameters,
                                                glib::DBusProxy::CallFinishedCallback const& cb)
{
  auto proxy = std::make_shared<glib::DBusProxy>(test_mode_ ? testing::DBUS_NAME : "org.gnome.SessionManager",
                                                 "/org/gnome/SessionManager", "org.gnome.SessionManager");

  // By passing the proxy to the lambda we ensure that it will be smartly handled
  proxy->CallBegin(method, parameters, [proxy, cb] (GVariant* ret, glib::Error const& e) {
    if (e)
    {
      LOG_ERROR(logger) << "Gnome Session call failed: " << e.Message();
    }

    if (cb)
      cb(ret, e);
  });
}

void GnomeManager::Impl::CallUPowerMethod(std::string const& method, glib::DBusProxy::ReplyCallback const& cb)
{
  auto proxy = std::make_shared<glib::DBusProxy>(test_mode_ ? testing::DBUS_NAME : "org.freedesktop.UPower",
                                                 "/org/freedesktop/UPower", "org.freedesktop.UPower",
                                                 test_mode_ ? G_BUS_TYPE_SESSION : G_BUS_TYPE_SYSTEM);

  proxy->CallBegin(method, nullptr, [proxy, cb] (GVariant *ret, glib::Error const& e) {
    if (e)
    {
      LOG_ERROR(logger) << "UPower call failed: " << e.Message();
    }
    else if (cb)
    {
      cb(ret);
    }
  });
}

void GnomeManager::Impl::CallLogindMethod(std::string const& method, GVariant* parameters, glib::DBusProxy::CallFinishedCallback const& cb)
{
  auto proxy = std::make_shared<glib::DBusProxy>(test_mode_ ? testing::DBUS_NAME : "org.freedesktop.login1",
                                                 "/org/freedesktop/login1",
                                                 "org.freedesktop.login1.Manager",
                                                 test_mode_ ? G_BUS_TYPE_SESSION : G_BUS_TYPE_SYSTEM);

  // By passing the proxy to the lambda we ensure that it will be smartly handled
  proxy->CallBegin(method, parameters, [proxy, cb, method] (GVariant* ret, glib::Error const& e) {
    if (e)
    {
      LOG_ERROR(logger) << "logind " << method << " call failed: " << e.Message();
    }

    if (cb)
    {
      cb(ret, e);
    }
  });
}

void GnomeManager::Impl::CallConsoleKitMethod(std::string const& method, GVariant* parameters)
{
  auto proxy = std::make_shared<glib::DBusProxy>(test_mode_ ? testing::DBUS_NAME : "org.freedesktop.ConsoleKit",
                                                 "/org/freedesktop/ConsoleKit/Manager",
                                                 "org.freedesktop.ConsoleKit.Manager",
                                                 test_mode_ ? G_BUS_TYPE_SESSION : G_BUS_TYPE_SYSTEM);

  // By passing the proxy to the lambda we ensure that it will be smartly handled
  proxy->CallBegin(method, parameters, [this, proxy] (GVariant*, glib::Error const& e) {
    if (e)
    {
      LOG_ERROR(logger) << "Fallback call failed: " << e.Message();
    }
  });
}

// Public implementation

GnomeManager::GnomeManager()
  : impl_(new Impl(this))
{}

GnomeManager::GnomeManager(GnomeManager::TestMode const& tm)
  : impl_(new Impl(this, true))
{}

GnomeManager::~GnomeManager()
{}

std::string GnomeManager::RealName() const
{
  const char* name = g_get_real_name();

  std::string real_name(name ? name : "");

  if (real_name == "Unknown")
    real_name.clear();

  return real_name;
}

std::string GnomeManager::UserName() const
{
  const char* name = g_get_user_name();

  return name ? name : "";
}

void GnomeManager::LockScreen()
{
  impl_->EnsureCancelPendingAction();

  // FIXME (andy) we should ask gnome-session to emit the logind signal
  lock_requested.emit();
}

void GnomeManager::Logout()
{
  enum class LogoutMethods : unsigned
  {
    INTERACTIVE = 0,
    NO_CONFIRMATION,
    FORCE_LOGOUT
  };

  auto mode = LogoutMethods::NO_CONFIRMATION;

  impl_->EnsureCancelPendingAction();
  impl_->pending_action_ = shell::Action::LOGOUT;
  impl_->CallGnomeSessionMethod("Logout", g_variant_new("(u)", mode),
    [this] (GVariant*, glib::Error const& err) {
      if (err)
      {
        LOG_WARNING(logger) << "Got error during call: " << err.Message();

        impl_->pending_action_ = shell::Action::NONE;
        // fallback to logind
        const char* session_id = g_getenv("XDG_SESSION_ID");

        auto call_consolekit_lambda = [this] {
          // fallback to ConsoleKit
          const char* cookie = g_getenv("XDG_SESSION_COOKIE");

          if (cookie && cookie[0] != '\0')
            impl_->CallConsoleKitMethod("CloseSession", g_variant_new("(s)", cookie));
        };

        if (session_id && session_id[0] != '\0')
        {
          impl_->CallLogindMethod("TerminateSession", g_variant_new("(s)", session_id), [call_consolekit_lambda] (GVariant*, glib::Error const& err) {
            if (err)
              call_consolekit_lambda();
          });
        }
        else
        {
          call_consolekit_lambda();
        }
      }
  });
}

void GnomeManager::Reboot()
{
  impl_->EnsureCancelPendingAction();
  impl_->pending_action_ = shell::Action::REBOOT;
  impl_->CallGnomeSessionMethod("Reboot", nullptr,
    [this] (GVariant*, glib::Error const& err) {
      if (err)
      {
        LOG_WARNING(logger) << "Got error during call: " << err.Message()
                            << ". Using fallback method";

        impl_->pending_action_ = shell::Action::NONE;
        // logind fallback
        impl_->CallLogindMethod("Reboot", g_variant_new("(b)", FALSE), [this] (GVariant* variant, glib::Error const& err) {
         // ConsoleKit fallback
         if (err)
           impl_->CallConsoleKitMethod("Restart");
         });
      }
  });
}

void GnomeManager::Shutdown()
{
  impl_->EnsureCancelPendingAction();
  impl_->pending_action_ = shell::Action::SHUTDOWN;
  impl_->CallGnomeSessionMethod("Shutdown", nullptr,
    [this] (GVariant*, glib::Error const& err) {
      if (err)
      {
        LOG_WARNING(logger) << "Got error during call: " << err.Message()
                            << ". Using fallback method";

        impl_->pending_action_ = shell::Action::NONE;
        // logind fallback
        impl_->CallLogindMethod("PowerOff", g_variant_new("(b)", FALSE), [this] (GVariant* variant, glib::Error const& err) {
         // ConsoleKit fallback
         if (err)
           impl_->CallConsoleKitMethod("Stop");
         });
      }
  });
}

void GnomeManager::Suspend()
{
  impl_->EnsureCancelPendingAction();
  impl_->CallLogindMethod("Suspend", g_variant_new("(b)", FALSE), [this] (GVariant* variant, glib::Error const& err) {
    // fallback to UPower
    if (err) 
      impl_->CallUPowerMethod("Suspend");
  });
}

void GnomeManager::Hibernate()
{
  impl_->EnsureCancelPendingAction();
  impl_->CallLogindMethod("Hibernate", g_variant_new("(b)", FALSE), [this] (GVariant* variant, glib::Error const& err) {
    // fallback to UPower
    if (err)
      impl_->CallUPowerMethod("Hibernate");
  });
}

bool GnomeManager::CanShutdown() const
{
  return impl_->can_shutdown_;
}

bool GnomeManager::CanSuspend() const
{
  return impl_->can_suspend_;
}

bool GnomeManager::CanHibernate() const
{
  return impl_->can_hibernate_;
}

void GnomeManager::CancelAction()
{
  impl_->CancelAction();
}

} // namespace session
} // namespace unity
