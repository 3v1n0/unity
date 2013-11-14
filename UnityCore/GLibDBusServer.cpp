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
GDBusInterfaceInfo* safe_interface_info_ref(GDBusInterfaceInfo* info)
{
  if (info) ::g_dbus_interface_info_ref(info);

  return info;
}

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

struct DBusObject::Impl
{
  Impl(DBusObject* obj, std::string const& introspection_xml, std::string const& interface_name)
    : object_(obj)
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

    auto* iface_info = g_dbus_node_info_lookup_interface(node_info.get(), interface_name.c_str());
    interface_info_.reset(safe_interface_info_ref(iface_info), safe_interface_info_unref);

    if (!interface_info_)
    {
      LOG_ERROR(logger_o) << "Unable to find the interface '" << interface_name
                          << "' in the provided introspection XML";
      return;
    }


    interface_vtable_.method_call = [] (GDBusConnection* connection, const gchar* sender,
                                        const gchar* object_path, const gchar* interface_name,
                                        const gchar* method_name, GVariant* parameters,
                                        GDBusMethodInvocation* invocation, gpointer user_data) {
      auto self = static_cast<DBusObject::Impl*>(user_data);
      GVariant *ret = nullptr;

      if (self->method_cb_)
      {
        ret = self->method_cb_(method_name ? method_name : "", parameters);

        LOG_INFO(logger_o) << "Called method: '" << method_name << " " << parameters
                           << "' on object '" << object_path << "' with interface '"
                           << interface_name << "' , returning: '" << ret << "'";

        const GDBusMethodInfo* info = g_dbus_method_invocation_get_method_info(invocation);

        if ((!ret || g_variant_equal(ret, glib::Variant(g_variant_new("()")))) && info->out_args && info->out_args[0])
        {
          LOG_ERROR(logger_o) << "Retuning NULL on method call '" << method_name << "' "
                              << "while its interface requires a value";

          std::string error_name = std::string(interface_name)+".Error.BadReturn";
          std::string error = "Returning invalid value for '"+std::string(method_name)+"' on path '"+std::string(object_path)+"'.";
          g_dbus_method_invocation_return_dbus_error(invocation, error_name.c_str(), error.c_str());

          if (ret)
            g_variant_unref (ret);
        }
        else
        {
          g_dbus_method_invocation_return_value(invocation, ret);
        }
      }
      else
      {
        LOG_WARN(logger_o) << "Called method: '" << method_name << " " << parameters
                           << "' on object '" << object_path << "' with interface '"
                           << interface_name << "', but no methods handler is set";

        std::string error_name = std::string(interface_name)+".Error.NoHandlerSet";
        std::string error = "No handler set for method '"+std::string(method_name)+"' on path '"+std::string(object_path)+"'.";
        g_dbus_method_invocation_return_dbus_error(invocation, error_name.c_str(), error.c_str());
      }
    };

    interface_vtable_.get_property = [] (GDBusConnection* connection, const gchar* sender,
                                         const gchar* object_path, const gchar* interface_name,
                                         const gchar* property_name, GError **error, gpointer user_data) {
      auto self = static_cast<DBusObject::Impl*>(user_data);
      GVariant *value = nullptr;

      if (self->property_get_cb_)
        value = self->property_get_cb_(property_name ? property_name : "");

      LOG_INFO(logger_o) << "Getting property '" << property_name << "' on '"
                         << interface_name << "' , returning: '" << value << "'";

      return value;
    };

    interface_vtable_.set_property = [] (GDBusConnection* connection, const gchar* sender,
                                         const gchar* object_path, const gchar* interface_name,
                                         const gchar* property_name, GVariant *value,
                                         GError **error, gpointer user_data) {
      auto self = static_cast<DBusObject::Impl*>(user_data);
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
                           << interface_name << "' , to value: '" << value << "'";

        if (old_value && !g_variant_equal(old_value, value))
          self->EmitPropertyChanged(property_name ? property_name : "");
      }
      else
      {
        LOG_WARN(logger_o) << "It was impossible to set the property '"
                           << property_name << "' on '" << interface_name
                           << "' , to value: '" << value << "'";
      }

      return ret;
    };
  }

  ~Impl()
  {
    UnRegister();
  }

  std::string InterfaceName() const
  {
    if (interface_info_ && interface_info_->name)
      return interface_info_->name;

    return "";
  }

  bool Register(glib::Object<GDBusConnection> const& conn, std::string const& path)
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
    object_->registered.emit(path);

    LOG_INFO(logger_o) << "Registering object '" << InterfaceName() << "'";

    return true;
  }

  void UnRegister(std::string const& path = "")
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
          object_->unregistered.emit(path);

          LOG_INFO(logger_o) << "Unregistering object '" << InterfaceName() << "'"
                             << " on path '" << path << "'";
          break;
        }
      }

      registrations_.erase(registration_id);
      connection_by_path_.erase(conn_it);

      if (registrations_.empty())
        object_->fully_unregistered.emit();
    }
    else
    {
      for (auto const& pair : registrations_)
      {
        auto const& registration_id = pair.first;
        auto const& obj_path = pair.second;
        auto const& connection = connection_by_path_[obj_path];

        g_dbus_connection_unregister_object(connection, registration_id);
        object_->unregistered.emit(obj_path);

        LOG_INFO(logger_o) << "Unregistering object '" << InterfaceName() << "'"
                           << " on path '" << obj_path << "'";
      }

      registrations_.clear();
      connection_by_path_.clear();
      object_->fully_unregistered.emit();
    }
  }

  void EmitGenericSignal(glib::Object<GDBusConnection> const& conn, std::string const& path,
                         std::string const& interface, std::string const& signal,
                         GVariant* parameters = nullptr)
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

  void EmitSignal(std::string const& signal, GVariant* parameters, std::string const& path)
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

  void EmitPropertyChanged(std::string const& property, std::string const& path = "")
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

    GVariant* value = property_get_cb_(property.c_str());

    if (!value)
    {
      LOG_ERROR(logger_o) << "The property value is not valid";
      return;
    }

    auto builder = g_variant_builder_new(G_VARIANT_TYPE_ARRAY);
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

  DBusObject* object_;
  MethodCallback method_cb_;
  PropertyGetterCallback property_get_cb_;
  PropertySetterCallback property_set_cb_;

  GDBusInterfaceVTable interface_vtable_;
  std::shared_ptr<GDBusInterfaceInfo> interface_info_;
  std::map<guint, std::string> registrations_;
  std::map<std::string, glib::Object<GDBusConnection>> connection_by_path_;
};

DBusObject::DBusObject(std::string const& introspection_xml, std::string const& interface_name)
  : impl_(new DBusObject::Impl(this, introspection_xml, interface_name))
{}

DBusObject::~DBusObject()
{}

void DBusObject::SetMethodsCallsHandler(MethodCallback const& func)
{
  impl_->method_cb_ = func;
}

void DBusObject::SetPropertyGetter(PropertyGetterCallback const& func)
{
  impl_->property_get_cb_ = func;
}

void DBusObject::SetPropertySetter(PropertySetterCallback const& func)
{
  impl_->property_set_cb_ = func;
}

std::string DBusObject::InterfaceName() const
{
  return impl_->InterfaceName();
}

bool DBusObject::Register(glib::Object<GDBusConnection> const& conn, std::string const& path)
{
  return impl_->Register(conn, path);
}

void DBusObject::UnRegister(std::string const& path)
{
  impl_->UnRegister(path);
}

void DBusObject::EmitSignal(std::string const& signal, GVariant* parameters, std::string const& path)
{
  impl_->EmitSignal(signal, parameters, path);
}

void DBusObject::EmitPropertyChanged(std::string const& property, std::string const& path)
{
  impl_->EmitPropertyChanged(property, path);
}

// DBusObjectBuilder

DECLARE_LOGGER(logger_b, "unity.glib.dbus.object.builder");

std::list<DBusObject::Ptr> DBusObjectBuilder::GetObjectsForIntrospection(std::string const& xml)
{
  std::list<DBusObject::Ptr> objects;
  glib::Error error;
  auto xml_int = g_dbus_node_info_new_for_xml(xml.c_str(), &error);
  std::shared_ptr<GDBusNodeInfo> node_info(xml_int, safe_node_info_unref);

  if (error || !node_info)
  {
    LOG_ERROR(logger_b) << "Unable to parse the given introspection: "
                        << error.Message();

    return objects;
  }

  for (unsigned i = 0; node_info->interfaces[i]; ++i)
  {
    glib::Error error;
    GDBusInterfaceInfo *interface = node_info->interfaces[i];

    auto obj = std::make_shared<DBusObject>(xml, interface->name);
    objects.push_back(obj);
  }

  return objects;
}

// DBusServer

DECLARE_LOGGER(logger_s, "unity.glib.dbus.server");

struct DBusServer::Impl
{
  Impl(DBusServer* server, std::string const& name = "")
    : server_(server)
    , name_(name)
    , name_owned_(false)
    , owner_name_(0)
  {}

  Impl(DBusServer* server, std::string const& name, GBusType bus_type)
    : Impl(server, name)
  {
    owner_name_ = g_bus_own_name(bus_type, name.c_str(), G_BUS_NAME_OWNER_FLAGS_NONE,
      [] (GDBusConnection* conn, const gchar* name, gpointer data)
      {
        auto self = static_cast<DBusServer::Impl*>(data);

        LOG_INFO(logger_s) << "DBus name acquired '" << name << "'";

        self->connection_ = glib::Object<GDBusConnection>(conn, glib::AddRef());
        self->name_owned_ = true;
        self->server_->name_acquired.emit();
        self->server_->connected.emit();
        self->RegisterPendingObjects();
      }, nullptr,
      [] (GDBusConnection *connection, const gchar *name, gpointer data)
      {
        auto self = static_cast<DBusServer::Impl*>(data);

        LOG_ERROR(logger_s) << "DBus name lost '" << name << "'";

        self->name_owned_ = false;
        self->server_->name_lost.emit();
      }, this, nullptr);
  }

  Impl(DBusServer* server, GBusType bus_type)
    : Impl(server)
  {
    g_bus_get(bus_type, cancellable_, [] (GObject*, GAsyncResult* res, gpointer data) {
      auto self = static_cast<DBusServer::Impl*>(data);
      glib::Error error;

      GDBusConnection* conn = g_bus_get_finish(res, &error);

      if (!error)
      {
        self->connection_ = glib::Object<GDBusConnection>(conn, glib::AddRef());
        self->server_->connected.emit();
        self->RegisterPendingObjects();
      }
      else
      {
        LOG_ERROR(logger_s) << "Can't get bus: " << error.Message();
      }
    }, this);
  }

  ~Impl()
  {
    if (owner_name_)
      g_bus_unown_name(owner_name_);

    LOG_INFO(logger_s) << "Removing dbus server";
  }

  void RegisterPendingObjects()
  {
    for (auto const& pair : pending_objects_)
      AddObject(pair.first, pair.second);

    pending_objects_.clear();
  }

  bool AddObject(DBusObject::Ptr const& obj, std::string const& path)
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

  bool RemoveObject(DBusObject::Ptr const& obj)
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
      auto it = std::find(objects_.begin(), objects_.end(), obj);
      if (it != objects_.end())
      {
        objects_.erase(it);
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

  DBusObject::Ptr GetObject(std::string const& interface) const
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

  void EmitSignal(std::string const& interface, std::string const& signal, GVariant* parameters)
  {
    auto const& obj = GetObject(interface);

    if (obj)
      obj->EmitSignal(signal, parameters);
  }

  DBusServer* server_;
  std::string name_;
  bool name_owned_;
  guint owner_name_;
  glib::Cancellable cancellable_;
  glib::Object<GDBusConnection> connection_;
  std::vector<DBusObject::Ptr> objects_;
  std::vector<std::pair<DBusObject::Ptr, std::string>> pending_objects_;
};

DBusServer::DBusServer(std::string const& name, GBusType bus_type)
  : impl_(new DBusServer::Impl(this, name, bus_type))
{}

DBusServer::DBusServer(GBusType bus_type)
  : impl_(new DBusServer::Impl(this, bus_type))
{}

DBusServer::~DBusServer()
{}

bool DBusServer::IsConnected() const
{
  return impl_->connection_.IsType(G_TYPE_DBUS_CONNECTION);
}

std::string const& DBusServer::Name() const
{
  return impl_->name_;
}

bool DBusServer::OwnsName() const
{
  return impl_->name_owned_;
}

void DBusServer::AddObjects(std::string const& introspection_xml, std::string const& path)
{
  auto const& objs = DBusObjectBuilder::GetObjectsForIntrospection(introspection_xml);

  if (objs.empty())
  {
    LOG_WARN(logger_s) << "Impossible to add empty objects list";
  }
  else
  {
    for (auto const& obj : objs)
      AddObject(obj, path);
  }
}

bool DBusServer::AddObject(DBusObject::Ptr const& obj, std::string const& path)
{
  return impl_->AddObject(obj, path);
}

bool DBusServer::RemoveObject(DBusObject::Ptr const& obj)
{
  return impl_->RemoveObject(obj);
}

std::list<DBusObject::Ptr> DBusServer::GetObjects() const
{
  std::list<DBusObject::Ptr> objects;

  for (auto const& pair : impl_->pending_objects_)
    objects.push_back(pair.first);

  for (auto const& obj : impl_->objects_)
    objects.push_back(obj);

  return objects;
}

DBusObject::Ptr DBusServer::GetObject(std::string const& interface) const
{
  return impl_->GetObject(interface);
}

void DBusServer::EmitSignal(std::string const& interface, std::string const& signal, GVariant* parameters)
{
  impl_->EmitSignal(interface, signal, parameters);
}

} // namespace glib
} // namespace unity
