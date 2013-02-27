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
const char* DBUS_NAME = "org.gnome.Shell";
const char* DBUS_INTERFACE = "org.gnome.SessionManager.EndSessionDialog";
const char* DBUS_OBJECT_PATH = "/org/gnome/SessionManager/EndSessionDialog";
const char* INTROSPECTION_XML =
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

GDBusInterfaceVTable INTERFACE_VTABLE =
{
  [] (GDBusConnection* connection, const gchar* sender, const gchar* object_path,
      const gchar* interface_name, const gchar* method_name, GVariant* parameters,
      GDBusMethodInvocation* invocation, gpointer user_data) {
        auto impl = static_cast<GnomeManager::Impl*>(user_data);
        impl->OnShellMethodCall(method_name ? method_name : "", parameters);
        g_dbus_method_invocation_return_value(invocation, nullptr);
  },
  nullptr, nullptr
};

}

GnomeManager::Impl::Impl(GnomeManager* manager)
  : manager_(manager)
  , can_shutdown_(true)
  , can_suspend_(false)
  , can_hibernate_(false)
  , shell_owner_name_(0)
  , pending_action_(shell::Action::NONE)
{
  SetupShellSessionHandler();

  CallUPowerMethod("HibernateAllowed", [this] (GVariant* variant) {
    can_hibernate_ = glib::Variant(variant).GetBool();
    LOG_INFO(logger) << "Can hibernate: " << can_hibernate_;
  });

  CallUPowerMethod("SuspendAllowed", [this] (GVariant* variant) {
    can_suspend_ = glib::Variant(variant).GetBool();
    LOG_INFO(logger) << "Can suspend: " << can_suspend_;
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
  TearDownShellSessionHandler();
}

void GnomeManager::Impl::SetupShellSessionHandler()
{
  shell_owner_name_ = g_bus_own_name(G_BUS_TYPE_SESSION, shell::DBUS_NAME,
                                     G_BUS_NAME_OWNER_FLAGS_REPLACE,
    [] (GDBusConnection* conn, const gchar* name, gpointer data)
    {
      auto self = static_cast<GnomeManager::Impl*>(data);
      auto xml_int = g_dbus_node_info_new_for_xml(shell::INTROSPECTION_XML, nullptr);
      std::shared_ptr<GDBusNodeInfo> introspection(xml_int, g_dbus_node_info_unref);

      if (!introspection)
      {
        LOG_ERROR(logger) << "No dbus introspection data could be loaded."
                          << "Session manager integration will not work";
        return;
      }

      for (unsigned i = 0; introspection->interfaces[i]; ++i)
      {
        glib::Error error;
        GDBusInterfaceInfo *interface = introspection->interfaces[i];

        unsigned id = g_dbus_connection_register_object(conn, shell::DBUS_OBJECT_PATH, interface,
                                                        &shell::INTERFACE_VTABLE, data, nullptr, &error);
        if (error)
        {
          LOG_ERROR(logger) << "Could not register debug interface onto d-bus: "
                            << error.Message();
        }
        else
        {
          self->shell_objects_ids_.push_back(id);
        }
      }

      self->shell_connection_ = glib::Object<GDBusConnection>(conn, glib::AddRef());

    }, nullptr, [] (GDBusConnection *connection, const gchar *name, gpointer user_data)
    {
      LOG_ERROR(logger) << "DBus name Lost " << name;
    }
    , this, nullptr);
}

void GnomeManager::Impl::TearDownShellSessionHandler()
{
  CancelAction();
  ClosedDialog();

  for (auto id : shell_objects_ids_)
    g_dbus_connection_unregister_object(shell_connection_, id);

  if (shell_owner_name_)
    g_bus_unown_name(shell_owner_name_);
}

void GnomeManager::Impl::OnShellMethodCall(std::string const& method, GVariant* parameters)
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
    bool has_inibitors = (g_variant_iter_next_value(inhibitors) != nullptr);
    g_variant_iter_free(inhibitors);

    LOG_INFO(logger) << "Got Open request for action " << unsigned(action)
                     << " with inhibitors " << has_inibitors;

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
}

void GnomeManager::Impl::EmitShellSignal(std::string const& signal, GVariant* parameters)
{
  glib::Error error;

  pending_action_ = shell::Action::NONE;

  LOG_INFO(logger) << "Emitting Shell signal '" << signal << "'";
  g_dbus_connection_emit_signal(shell_connection_, nullptr, shell::DBUS_OBJECT_PATH,
                                shell::DBUS_INTERFACE, signal.c_str(), parameters, &error);

  if (error)
  {
    LOG_WARNING(logger) << "Got error when emitting signal '" << signal << "': "
                        << error.Message();
  }
}

void GnomeManager::Impl::ConfirmLogout()
{
  EmitShellSignal("ConfirmedLogout");
}

void GnomeManager::Impl::ConfirmReboot()
{
  EmitShellSignal("ConfirmedReboot");
}

void GnomeManager::Impl::ConfirmShutdown()
{
  EmitShellSignal("ConfirmedShutdown");
}

void GnomeManager::Impl::CancelAction()
{
  EmitShellSignal("Canceled");
}

void GnomeManager::Impl::ClosedDialog()
{
  EmitShellSignal("Closed");
}

void GnomeManager::Impl::CallGnomeSessionMethod(std::string const& method, GVariant* parameters,
                                                glib::DBusProxy::CallFinishedCallback const& cb)
{
  auto proxy = std::make_shared<glib::DBusProxy>("org.gnome.SessionManager",
                                                 "/org/gnome/SessionManager",
                                                 "org.gnome.SessionManager");

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
  auto proxy = std::make_shared<glib::DBusProxy>("org.freedesktop.UPower", "/org/freedesktop/UPower",
                                                 "org.freedesktop.UPower", G_BUS_TYPE_SYSTEM);

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

void GnomeManager::Impl::CallConsoleKitMethod(std::string const& method, GVariant* parameters)
{
  auto proxy = std::make_shared<glib::DBusProxy>("org.freedesktop.ConsoleKit",
                                                 "/org/freedesktop/ConsoleKit/Manager",
                                                 "org.freedesktop.ConsoleKit.Manager",
                                                 G_BUS_TYPE_SYSTEM);

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
  : impl_(new GnomeManager::Impl(this))
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
  if (impl_->pending_action_ != shell::Action::NONE)
    CancelAction();

  auto proxy = std::make_shared<glib::DBusProxy>("org.gnome.ScreenSaver",
                                                 "/org/gnome/ScreenSaver",
                                                 "org.gnome.ScreenSaver");

  // By passing the proxy to the lambda we ensure that it will stay alive
  // until we get the last callback.
  proxy->Call("Lock", nullptr, [proxy] (GVariant*) {});
  proxy->Call("SimulateUserActivity", nullptr, [proxy] (GVariant*) {});
}

void GnomeManager::Logout()
{
  if (impl_->pending_action_ != shell::Action::NONE)
  {
    CancelAction();
  }

  enum class LogoutMethods : unsigned
  {
    INTERACTIVE = 0,
    NO_CONFIRMATION,
    FORCE_LOGOUT
  };

  auto mode = LogoutMethods::NO_CONFIRMATION;

  impl_->pending_action_ = shell::Action::LOGOUT;
  impl_->CallGnomeSessionMethod("Logout", g_variant_new("(u)", mode),
    [this] (GVariant*, glib::Error const& err) {
      if (err)
      {
        LOG_WARNING(logger) << "Got error during call: " << err.Message();

        impl_->pending_action_ = shell::Action::NONE;
        const char* cookie = g_getenv("XDG_SESSION_COOKIE");

        if (cookie && cookie[0] != '\0')
          impl_->CallConsoleKitMethod("CloseSession", g_variant_new("(s)", cookie));
      }
  });
}

void GnomeManager::Reboot()
{
  if (impl_->pending_action_ != shell::Action::NONE)
  {
    CancelAction();
  }

  impl_->pending_action_ = shell::Action::REBOOT;
  impl_->CallGnomeSessionMethod("RequestReboot", nullptr,
    [this] (GVariant*, glib::Error const& err) {
      if (err)
      {
        LOG_WARNING(logger) << "Got error during call: " << err.Message()
                            << ". Using fallback method";

        impl_->pending_action_ = shell::Action::NONE;
        impl_->CallConsoleKitMethod("Restart");
      }
  });
}

void GnomeManager::Shutdown()
{
  if (impl_->pending_action_ != shell::Action::NONE)
  {
    CancelAction();
  }

  impl_->pending_action_ = shell::Action::SHUTDOWN;
  impl_->CallGnomeSessionMethod("RequestShutdown", nullptr,
    [this] (GVariant*, glib::Error const& err) {
      if (err)
      {
        LOG_WARNING(logger) << "Got error during call: " << err.Message()
                            << ". Using fallback method";

        impl_->pending_action_ = shell::Action::NONE;
        impl_->CallConsoleKitMethod("Stop");
      }
  });
}

void GnomeManager::Suspend()
{
  impl_->CancelAction();
  impl_->CallUPowerMethod("Suspend");
}

void GnomeManager::Hibernate()
{
  impl_->CancelAction();
  impl_->CallUPowerMethod("Hibernate");
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
