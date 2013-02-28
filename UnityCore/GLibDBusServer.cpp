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
#include "Variant.h"

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
    glib::Variant old_value;
    gboolean ret = TRUE;

    if (self->property_get_cb_)
      old_value = self->property_get_cb_(property_name ? property_name : "");

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

      if (!g_variant_equal(old_value, value))
        self->EmitPropertyChanged(property_name ? property_name : "");
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

DBusObject::~DBusObject()
{
  UnRegister();
}

void DBusObject::SetMethodsCallsHandler(MethodCallback const& func)
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

bool DBusObject::Register(glib::Object<GDBusConnection> const& conn, std::string const& path)
{
  if (!interface_info_)
  {
    LOG_ERROR(logger_o) << "Can't register object '" << InterfaceName()
                        << "', bad interface";
    return false;
  }

  if (connection_by_path_.find(path) != connection_by_path_.end())
  {
    LOG_ERROR(logger_o) << "Can't register object '" << InterfaceName()
                        << "', it's already registered on path '" << path << "'";
    return false;
  }

  if (!conn.IsType(G_TYPE_DBUS_CONNECTION))
  {
    LOG_ERROR(logger_o) << "Can't register object '" << InterfaceName()
                        << "', invalid connection";
    return false;
  }

  glib::Error error;

  guint id = g_dbus_connection_register_object(conn, path.c_str(), interface_info_.get(),
                                               &interface_vtable_, this, nullptr, &error);
  if (error)
  {
    LOG_ERROR(logger_o) << "Could not register object in dbus: "
                        << error.Message();
    return false;
  }

  registrations_[id] = path;
  connection_by_path_[path] = conn;
  registered.emit(path);

  LOG_INFO(logger_o) << "Registering object '" << InterfaceName() << "'";

  return true;
}

void DBusObject::UnRegister(std::string const& path)
{
  if (!path.empty())
  {
    auto conn_it = connection_by_path_.find(path);

    if (conn_it == connection_by_path_.end())
    {
      LOG_WARN(logger_o) << "Impossible unregistering object for path " << path;
      return;
    }

    guint registration_id = 0;

    for (auto const& pair : registrations_)
    {
      auto const& obj_path = pair.second;

      if (obj_path == path)
      {
        registration_id = pair.first;
        g_dbus_connection_unregister_object(conn_it->second, registration_id);
        unregistered.emit(path);

        LOG_INFO(logger_o) << "Unregistering object '" << InterfaceName() << "'"
                           << " on path '" << path << "'";
        break;
      }
    }

    registrations_.erase(registration_id);
    connection_by_path_.erase(conn_it);

    if (registrations_.empty())
      fully_unregistered.emit();
  }
  else
  {
    for (auto const& pair : registrations_)
    {
      auto const& registration_id = pair.first;
      auto const& obj_path = pair.second;
      auto connection = connection_by_path_[obj_path];

      g_dbus_connection_unregister_object(connection, registration_id);
      unregistered.emit(obj_path);

      LOG_INFO(logger_o) << "Unregistering object '" << InterfaceName() << "'"
                         << " on path '" << obj_path << "'";
    }

    registrations_.clear();
    connection_by_path_.clear();
    fully_unregistered.emit();
  }
}

void DBusObject::EmitSignal(std::string const& signal, GVariant* parameters, std::string const& path)
{
  glib::Variant reffed_params(parameters);

  if (signal.empty())
  {
    LOG_ERROR(logger_o) << "Impossible to emit an empty signal";
    return;
  }

  if (!path.empty())
  {
    auto conn_it = connection_by_path_.find(path);

    if (conn_it == connection_by_path_.end())
    {
      LOG_ERROR(logger_o) << "Impossible to emit signal '" << signal << "' "
                          << "on object path '" << path << "': no connection available";
      return;
    }

    EmitGenericSignal(conn_it->second, path, InterfaceName(), signal, parameters);
  }
  else
  {
    for (auto const& pair : connection_by_path_)
    {
      glib::Variant params(parameters);
      auto const& obj_path = pair.first;
      auto const& conn = pair.second;

      EmitGenericSignal(conn, obj_path, InterfaceName(), signal, params);
    }
  }
}

void DBusObject::EmitPropertyChanged(std::string const& property, std::string const& path)
{
  if (property.empty())
  {
    LOG_ERROR(logger_o) << "Impossible to emit a changed property for an invalid one";
    return;
  }

  if (!property_get_cb_)
  {
    LOG_ERROR(logger_o) << "We don't have a property getter for this object";
    return;
  }

  auto builder = g_variant_builder_new(G_VARIANT_TYPE_ARRAY);
  GVariant* value = property_get_cb_(property.c_str());
  g_variant_builder_add(builder, "{sv}", property.c_str(), value);
  glib::Variant parameters(g_variant_new("(sa{sv}as)", InterfaceName().c_str(), builder, nullptr));

  if (!path.empty())
  {
    auto conn_it = connection_by_path_.find(path);

    if (conn_it == connection_by_path_.end())
    {
      LOG_ERROR(logger_o) << "Impossible to emit property changed '" << property << "' "
                          << "on object path '" << path << "': no connection available";
      return;
    }

    EmitGenericSignal(conn_it->second, path, "org.freedesktop.DBus.Properties", "PropertiesChanged", parameters);
  }
  else
  {
    for (auto const& pair : connection_by_path_)
    {
      glib::Variant reffed_params(parameters);
      auto const& obj_path = pair.first;
      auto const& conn = pair.second;

      EmitGenericSignal(conn, obj_path, "org.freedesktop.DBus.Properties", "PropertiesChanged", reffed_params);
    }
  }
}

void DBusObject::EmitGenericSignal(glib::Object<GDBusConnection> const& conn, std::string const& path, std::string const& interface, std::string const& signal, GVariant* parameters)
{
  LOG_INFO(logger_o) << "Emitting signal '" << signal << "'" << " for the interface "
                     << "'" << interface << "' on object path '" << path << "'";

  glib::Error error;
  g_dbus_connection_emit_signal(conn, nullptr, path.c_str(), interface.c_str(),
                                signal.c_str(), parameters, &error);

  if (error)
  {
    LOG_ERROR(logger_o) << "Got error when emitting signal '" << signal << "': "
                        << " for the interface '" << interface << "' on object path '"
                        << path << "': " << error.Message();
  }
}

// DBusServer

DECLARE_LOGGER(logger_s, "unity.glib.dbus.server");

DBusServer::DBusServer(std::string const& name, GBusType bus_type)
  : name_owned_(false)
  , owner_name_(0)
{
  owner_name_ = g_bus_own_name(bus_type, name.c_str(), G_BUS_NAME_OWNER_FLAGS_NONE,
    [] (GDBusConnection* conn, const gchar* name, gpointer data)
    {
      auto self = static_cast<DBusServer*>(data);

      LOG_INFO(logger_s) << "DBus name acquired '" << name << "'";

      self->connection_ = glib::Object<GDBusConnection>(conn, glib::AddRef());
      self->name_owned_ = true;
      self->name_acquired.emit();

      for (auto const& pair : self->pending_objects_)
        self->AddObject(pair.first, pair.second);

      self->pending_objects_.clear();

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
  if (owner_name_)
    g_bus_unown_name(owner_name_);

  LOG_INFO(logger_s) << "Removing dbus server";
}

bool DBusServer::AddObject(DBusObject::Ptr const& obj, std::string const& path)
{
  if (!obj)
  {
    LOG_ERROR(logger_s) << "Can't register an invalid object";
    return false;
  }

  if (!connection_)
  {
    LOG_WARN(logger_s) << "Can't register object '" << obj->InterfaceName()
                       << "' yet as we don't have a connection, waiting for it...";

    pending_objects_.push_back(std::make_pair(obj, path));
  }
  else
  {
    if (obj->Register(connection_, path))
    {
      objects_.push_back(obj);
      return true;
    }
  }

  return false;
}

bool DBusServer::RemoveObject(DBusObject::Ptr const& obj)
{
  if (!obj)
    return false;

  bool removed = false;

  for (auto it = pending_objects_.begin(); it != pending_objects_.end(); ++it)
  {
    if (it->first == obj)
    {
      pending_objects_.erase(it);
      removed = true;
      LOG_INFO(logger_s) << "Removing object '" << obj->InterfaceName() << "' ...";
      break;
    }
  }

  if (!removed)
  {
    if (std::remove(objects_.begin(), objects_.end(), obj) != objects_.end())
    {
      removed = true;
      LOG_INFO(logger_s) << "Removing object '" << obj->InterfaceName() << "' ...";
    }
  }

  if (removed)
  {
    obj->UnRegister();
  }

  return removed;
}

DBusObject::Ptr DBusServer::GetObject(std::string const& interface)
{
  for (auto const& pair : pending_objects_)
  {
    if (pair.first->InterfaceName() == interface)
      return pair.first;
  }

  for (auto const& obj : objects_)
  {
    if (obj->InterfaceName() == interface)
      return obj;
  }

  return DBusObject::Ptr();
}

} // namespace glib
} // namespace unity
