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

#include <NuxCore/Logger.h>

#include "GLibDBusServer.h"
#include "GLibWrapper.h"

namespace unity
{
namespace glib
{
namespace
{
void safe_interface_info_unref(GDBusInterfaceInfo* info)
{
  if (info) ::g_dbus_interface_info_unref(info);
}

void safe_node_info_unref(GDBusNodeInfo* info)
{
  if (info) ::g_dbus_node_info_unref(info);
}
}

DECLARE_LOGGER(logger_o, "unity.glib.dbus.object");

DBusObject::DBusObject(std::string const& introspection_xml, std::string const& interface_name)
  : interface_info_(nullptr, safe_interface_info_unref)
{
  glib::Error error;
  auto xml_int = g_dbus_node_info_new_for_xml(introspection_xml.c_str(), &error);
  std::shared_ptr<GDBusNodeInfo> node_info(xml_int, safe_node_info_unref);

  if (error)
  {
    LOG_ERROR(logger_o) << "Unable to parse the given introspection for "
                        << interface_name << ": " << error.Message();
  }

  if (!node_info)
    return;

  interface_info_.reset(g_dbus_node_info_lookup_interface(node_info.get(), interface_name.c_str()));

  if (!interface_info_)
  {
    LOG_ERROR(logger_o) << "Unable to find the interface '" << interface_name
                        << "' in the provided introspection XML";
    return;
  }

  g_dbus_interface_info_ref(interface_info_.get());

  interface_vtable_.method_call = [] (GDBusConnection* connection, const gchar* sender,
                                      const gchar* object_path, const gchar* interface_name,
                                      const gchar* method_name, GVariant* parameters,
                                      GDBusMethodInvocation* invocation, gpointer user_data) {
    auto self = static_cast<DBusObject*>(user_data);
    GVariant *ret = nullptr;

    if (self->method_cb_)
      ret = self->method_cb_(method_name ? method_name : "", parameters);

    LOG_INFO(logger_o) << "Called method: '" << method_name << " "
                       << (parameters ? g_variant_print(parameters, TRUE) : "()")
                       << "' on object '" << object_path << "' with interface '"
                       << interface_name << "' , returning: '"
                       << (ret ? g_variant_print(ret, TRUE) : "()") << "'";

    g_dbus_method_invocation_return_value(invocation, ret);
  };

  interface_vtable_.get_property = [] (GDBusConnection* connection, const gchar* sender,
                                       const gchar* object_path, const gchar* interface_name,
                                       const gchar* property_name, GError **error, gpointer user_data) {
    auto self = static_cast<DBusObject*>(user_data);
    GVariant *value = nullptr;

    if (self->property_get_cb_)
      value = self->property_get_cb_(property_name ? property_name : "");

    LOG_INFO(logger_o) << "Getting property '" << property_name << "' on '"
                       << interface_name << "' , returning: '"
                       << (value ? g_variant_print(value, TRUE) : "()") << "'";

    return value;
  };

  interface_vtable_.set_property = [] (GDBusConnection* connection, const gchar* sender,
                                       const gchar* object_path, const gchar* interface_name,
                                       const gchar* property_name, GVariant *value,
                                       GError **error, gpointer user_data) {
    auto self = static_cast<DBusObject*>(user_data);
    gboolean ret = TRUE;

    if (self->property_set_cb_)
    {
      if (!self->property_set_cb_(property_name ? property_name : "", value))
      {
        ret = FALSE;

        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "It was impossible " \
                     "to set the property '%s' on '%s'", property_name, interface_name);
      }
    }

    if (ret)
    {
      LOG_INFO(logger_o) << "Setting property '" << property_name << "' on '"
                         << interface_name << "' , to value: '"
                         << (value ? g_variant_print(value, TRUE) : "<null>") << "'";
    }
    else
    {
      LOG_WARN(logger_o) << "It was impossible to set the property '"
                         << property_name << "' on '" << interface_name
                         << "' , to value: '"
                         << (value ? g_variant_print(value, TRUE) : "()")
                         << "'";
    }

    return ret;
  };
}

void DBusObject::SetMethodsCallHandler(MethodCallback const& func)
{
  method_cb_ = func;
}

void DBusObject::SetPropertyGetter(PropertyGetterCallback const& func)
{
  property_get_cb_ = func;
}

void DBusObject::SetPropertySetter(PropertySetterCallback const& func)
{
  property_set_cb_ = func;
}

std::string DBusObject::InterfaceName() const
{
  if (interface_info_ && interface_info_->name)
    return interface_info_->name;

  return "";
}

std::shared_ptr<GDBusInterfaceInfo> DBusObject::InterfaceInfo() const
{
  return interface_info_;
}

const GDBusInterfaceVTable* DBusObject::InterfaceVTable() const
{
  return &interface_vtable_;
}


// DBusServer

DECLARE_LOGGER(logger_s, "unity.glib.dbus.server");

DBusServer::DBusServer(std::string const& name, GBusType bus_type)
  : owner_name_(0)
  , name_owned_(false)
{
  owner_name_ = g_bus_own_name(bus_type, name.c_str(), G_BUS_NAME_OWNER_FLAGS_NONE,
    [] (GDBusConnection* conn, const gchar* name, gpointer data)
    {
      auto self = static_cast<DBusServer*>(data);

      LOG_INFO(logger_s) << "DBus name acquired '" << name << "'";

      self->connection_ = glib::Object<GDBusConnection>(conn, glib::AddRef());
      self->name_owned_ = true;
      self->name_acquired.emit();

    }, nullptr, [] (GDBusConnection *connection, const gchar *name, gpointer data)
    {
      auto self = static_cast<DBusServer*>(data);

      LOG_ERROR(logger_s) << "DBus name lost '" << name << "'";

      self->name_owned_ = false;
      self->name_lost.emit();
    }
    , this, nullptr);
}

bool DBusServer::OwnsName() const
{
  return name_owned_;
}

DBusServer::~DBusServer()
{
  if (connection_)
  {
    for (auto const& pair : objects_)
      g_dbus_connection_unregister_object(connection_, pair.first);
  }

  if (owner_name_)
    g_bus_unown_name(owner_name_);
}

void DBusServer::AddObject(DBusObject::Ptr const& obj, std::string const& path)
{
  if (!obj || !obj->InterfaceInfo())
  {
    LOG_ERROR(logger_s) << "Could not register an invalid object";
    return;
  }

  if (!connection_)
  {
    LOG_WARN(logger_s) << "Can't register object '" << obj->InterfaceName()
                       << "' yet as we don't have a connection, waiting for it.";

    auto conn = std::make_shared<sigc::connection>();
    *conn = name_acquired.connect([this, obj, path, conn] {
      LOG_INFO(logger_s) << "Name Was acquired";
      AddObject(obj, path);
      conn->disconnect();
    });
  }
  else
  {
    glib::Error error;

    unsigned id = g_dbus_connection_register_object(connection_, path.c_str(),
                                                    obj->InterfaceInfo().get(),
                                                    obj->InterfaceVTable(), obj.get(),
                                                    nullptr, &error);
    if (error)
    {
      LOG_ERROR(logger_s) << "Could not register object in dbus: "
                          << error.Message();
    }
    else
    {
      LOG_INFO(logger_s) << "Registering object '" << obj->InterfaceName() << "'";
      objects_[id] = obj;
    }
  }
}

} // namespace glib
} // namespace unity
