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

enum class DIALOG_TYPE : unsigned
{
  LOGOUT = 0,
  SHUTDOWN,
  REBOOT,
};

}

GnomeManager::Impl::Impl(GnomeManager* manager)
  : manager_(manager)
  , can_shutdown_(true)
  , can_suspend_(false)
  , can_hibernate_(false)
  , shell_owner_name_(0)
  , upower_proxy_("org.freedesktop.UPower", "/org/freedesktop/UPower",
                  "org.freedesktop.UPower", G_BUS_TYPE_SYSTEM)
  , gsession_proxy_("org.gnome.SessionManager", "/org/gnome/SessionManager",
                    "org.gnome.SessionManager")
{
  SetupShellSessionHandler();
  QueryUPowerCapabilities();

  upower_proxy_.Connect("Changed", sigc::hide(sigc::mem_fun(this, &GnomeManager::Impl::QueryUPowerCapabilities)));

  gsession_proxy_.Call("CanShutdown", nullptr, [this] (GVariant* variant) {
    can_shutdown_ = false;
    if (variant)
      g_variant_get(variant, "(b)", &can_shutdown_);
  });
}

GnomeManager::Impl::~Impl()
{
  if (shell_owner_name_)
    g_bus_unown_name(shell_owner_name_);
}

void GnomeManager::Impl::QueryUPowerCapabilities()
{
  upower_proxy_.Call("HibernateAllowed", nullptr, [this] (GVariant* variant) {
    can_hibernate_ = false;
    if (variant)
      g_variant_get(variant, "(b)", &can_hibernate_);
  });

  upower_proxy_.Call("SuspendAllowed", nullptr, [this] (GVariant* variant) {
    can_suspend_ = false;
    if (variant)
      g_variant_get(variant, "(b)", &can_suspend_);
  });
}

void GnomeManager::Impl::SetupShellSessionHandler()
{
  shell_owner_name_ = g_bus_own_name(G_BUS_TYPE_SESSION, shell::DBUS_NAME,
                                     G_BUS_NAME_OWNER_FLAGS_REPLACE,
    [] (GDBusConnection* conn, const gchar* name, gpointer data)
    {
      auto xml_int = g_dbus_node_info_new_for_xml(shell::INTROSPECTION_XML, nullptr);
      std::shared_ptr<GDBusNodeInfo> introspection(xml_int, g_dbus_node_info_unref);

      if (!introspection)
      {
        LOG_ERROR(logger) << "No dbus introspection data could be loaded."
                          << "Session manager integration will not work";
        return;
      }

      for (unsigned i; introspection->interfaces[i]; ++i)
      {
        glib::Error error;
        GDBusInterfaceInfo *interface = introspection->interfaces[i];

        g_dbus_connection_register_object(conn, shell::DBUS_OBJECT_PATH, interface,
                                          &shell::INTERFACE_VTABLE, data, nullptr, &error);
        if (error)
        {
          LOG_ERROR(logger) << "Could not register debug interface onto d-bus: "
                            << error.Message();
        }
      }

      auto self = static_cast<GnomeManager::Impl*>(data);
      self->shell_connection_ = glib::Object<GDBusConnection>(conn, glib::AddRef());

    }, nullptr, [] (GDBusConnection *connection, const gchar *name, gpointer user_data)
    {
      LOG_ERROR(logger) << "DBus name Lost " << name;
    }
    , this, nullptr);
}

void GnomeManager::Impl::OnShellMethodCall(std::string const& method_name, GVariant* parameters)
{
  LOG_DEBUG(logger) << "Called method '" << method_name << "'";

  if (method_name == "Open")
  {
    shell::DIALOG_TYPE type;
    unsigned arg1, timeout_length;
    GVariantIter *inhibitors;
    g_variant_get(parameters, "(uuuao)", &type, &arg1, &timeout_length, &inhibitors);
    bool has_inibitors = (g_variant_iter_n_children(inhibitors) > 0);
    g_variant_iter_free(inhibitors);

    //XXX: we need to define the proper policy here
    if (has_inibitors)
    {
      manager_->ClosedDialog();
      return;
    }

    switch(type)
    {
      case shell::DIALOG_TYPE::LOGOUT:
        manager_->logout_requested.emit();
        break;
      case shell::DIALOG_TYPE::SHUTDOWN:
        manager_->shutdown_requested.emit();
        break;
      case shell::DIALOG_TYPE::REBOOT:
        manager_->reboot_requested.emit();
        break;
    }
  }
  else if (method_name == "Close")
  {
    manager_->cancel_requested.emit();
  }
}

void GnomeManager::Impl::EmitShellSignal(std::string const& signal_name, GVariant* parameters)
{
  glib::Error error;
  g_dbus_connection_emit_signal(shell_connection_, nullptr, shell::DBUS_OBJECT_PATH,
                                shell::DBUS_INTERFACE, signal_name.c_str(), parameters,
                                &error);

  if (error)
  {
    LOG_WARNING(logger) << "Got error when emitting signal '" << signal_name << "': "
                        << signal_name;
  }
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

void GnomeManager::Logout()
{
  enum LogoutMethods
  {
    INTERACTIVE = 0,
    NO_CONFIRMATION,
    FORCE_LOGOUT
  };

  unsigned mode = LogoutMethods::NO_CONFIRMATION;
  impl_->gsession_proxy_.Call("Logout", g_variant_new_uint32(mode));
}

void GnomeManager::Reboot()
{
  impl_->gsession_proxy_.Call("RequestReboot");
}

void GnomeManager::Shutdown()
{
  impl_->gsession_proxy_.Call("RequestShutdown");
}

void GnomeManager::Suspend()
{
  impl_->upower_proxy_.Call("Suspend");
}

void GnomeManager::Hibernate()
{
  impl_->upower_proxy_.Call("Hibernate");
}

void GnomeManager::ConfirmLogout()
{
  impl_->EmitShellSignal("ConfirmLogout");
}

void GnomeManager::ConfirmReboot()
{
  impl_->EmitShellSignal("ConfirmReboot");
}

void GnomeManager::ConfirmShutdown()
{
  impl_->EmitShellSignal("ConfirmShutdown");
}

void GnomeManager::CancelAction()
{
  impl_->EmitShellSignal("CancelAction");
}

void GnomeManager::ClosedDialog()
{
  impl_->EmitShellSignal("ClosedDialog");
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

} // namespace session
} // namespace unity
