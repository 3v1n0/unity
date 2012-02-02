// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011 Canonical Ltd
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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#include "GLibDBusProxy.h"

#include <map>
#include <memory>
#include <NuxCore/Logger.h>
#include <vector>

#include "GLibWrapper.h"
#include "GLibSignal.h"

namespace unity
{
namespace glib
{

namespace
{
nux::logging::Logger logger("unity.glib.dbusproxy");
}

using std::string;

struct CallData
{
  DBusProxy::ReplyCallback callback;
  DBusProxy::Impl* impl;
  std::string method_name;
};

class DBusProxy::Impl
{
public:
  typedef std::vector<ReplyCallback> Callbacks;
  typedef std::map<string, Callbacks> SignalHandlers;

  Impl(DBusProxy* owner,
       string const& name,
       string const& object_path,
       string const& interface_name,
       GBusType bus_type,
       GDBusProxyFlags flags);
  ~Impl();

  void StartReconnectionTimeout();
  void Connect();
  void OnProxySignal(GDBusProxy* proxy, char* sender_name, char* signal_name,
                     GVariant* parameters);
  void Call(string const& method_name,
            GVariant* parameters,
            ReplyCallback callback,
            GCancellable* cancellable,
            GDBusCallFlags flags,
            int timeout_msec);

  void Connect(string const& signal_name, ReplyCallback callback);
  bool IsConnected();

  static void OnNameAppeared(GDBusConnection* connection, const char* name,
                             const char* name_owner, gpointer impl);
  static void OnNameVanished(GDBusConnection* connection, const char* name, gpointer impl);
  static void OnProxyConnectCallback(GObject* source, GAsyncResult* res, gpointer impl);
  static void OnCallCallback(GObject* source, GAsyncResult* res, gpointer call_data);
 
  DBusProxy* owner_;
  string name_;
  string object_path_;
  string interface_name_;
  GBusType bus_type_;
  GDBusProxyFlags flags_;

  glib::Object<GDBusProxy> proxy_;
  glib::Object<GCancellable> cancellable_;
  guint watcher_id_;
  guint reconnect_timeout_id_;
  bool connected_;

  glib::Signal<void, GDBusProxy*, char*, char*, GVariant*> g_signal_connection_;

  SignalHandlers handlers_;
};

DBusProxy::Impl::Impl(DBusProxy* owner,
                      string const& name,
                      string const& object_path,
                      string const& interface_name,
                      GBusType bus_type,
                      GDBusProxyFlags flags)
  : owner_(owner)
  , name_(name)
  , object_path_(object_path)
  , interface_name_(interface_name)
  , bus_type_(bus_type)
  , flags_(flags)
  , cancellable_(g_cancellable_new())
  , watcher_id_(0)
  , reconnect_timeout_id_(0)
  , connected_(false)
{
  LOG_DEBUG(logger) << "Watching name " << name_;
  watcher_id_ = g_bus_watch_name(bus_type_,
                                 name.c_str(),
                                 G_BUS_NAME_WATCHER_FLAGS_AUTO_START,
                                 DBusProxy::Impl::OnNameAppeared,
                                 DBusProxy::Impl::OnNameVanished,
                                 this,
                                 NULL);
  StartReconnectionTimeout();
}

DBusProxy::Impl::~Impl()
{
  g_cancellable_cancel(cancellable_);
  if (watcher_id_)
    g_bus_unwatch_name(watcher_id_);
  if (reconnect_timeout_id_)
    g_source_remove(reconnect_timeout_id_);
}

void DBusProxy::Impl::OnNameAppeared(GDBusConnection* connection,
                                     const char* name,
                                     const char* name_owner,
                                     gpointer impl)
{
  DBusProxy::Impl* self = static_cast<DBusProxy::Impl*>(impl);
  LOG_DEBUG(logger) << self->name_ << " appeared";
}

void DBusProxy::Impl::OnNameVanished(GDBusConnection* connection,
                                     const char* name,
                                     gpointer impl)
{
  DBusProxy::Impl* self = static_cast<DBusProxy::Impl*>(impl);

  LOG_DEBUG(logger) << self->name_ << " vanished";

  self->connected_ = false;
  self->owner_->disconnected.emit();
}

void DBusProxy::Impl::StartReconnectionTimeout()
{
  LOG_DEBUG(logger) << "Starting reconnection timeout for " << name_;

  if (reconnect_timeout_id_)
    g_source_remove(reconnect_timeout_id_);

  auto callback = [](gpointer user_data) -> gboolean
  {
    DBusProxy::Impl* self = static_cast<DBusProxy::Impl*>(user_data);
    if (!self->proxy_)
      self->Connect();

    self->reconnect_timeout_id_ = 0;
    return FALSE;
  };
  reconnect_timeout_id_ = g_timeout_add_seconds(1, callback, this);
}

void DBusProxy::Impl::Connect()
{
  LOG_DEBUG(logger) << "Attempting to connect to " << name_;
  g_dbus_proxy_new_for_bus(bus_type_,
                           flags_,
                           NULL,
                           name_.c_str(),
                           object_path_.c_str(),
                           interface_name_.c_str(),
                           cancellable_,
                           DBusProxy::Impl::OnProxyConnectCallback,
                           this);
}

bool DBusProxy::Impl::IsConnected()
{
  return connected_;
}

void DBusProxy::Impl::OnProxyConnectCallback(GObject* source,
                                             GAsyncResult* res,
                                             gpointer impl)
{
  glib::Error error;
  glib::Object<GDBusProxy> proxy(g_dbus_proxy_new_for_bus_finish(res, &error));

  // If the async operation was cancelled, this callback will still be called and
  // therefore we should deal with the error before touching the impl pointer
  if (!proxy || error)
  {
    LOG_WARNING(logger) << "Unable to connect to proxy: " << error;
    return;
  }

  DBusProxy::Impl* self = static_cast<DBusProxy::Impl*>(impl);
  LOG_DEBUG(logger) << "Sucessfully created proxy: " << self->object_path_;

  self->proxy_ = proxy;
  self->g_signal_connection_.Connect(self->proxy_, "g-signal",
                                     sigc::mem_fun(self, &DBusProxy::Impl::OnProxySignal));
  self->connected_ = true;
  self->owner_->connected.emit();
}

void DBusProxy::Impl::OnProxySignal(GDBusProxy* proxy,
                                    char* sender_name,
                                    char* signal_name,
                                    GVariant* parameters)
{
  LOG_DEBUG(logger) << "Signal Received for proxy (" << object_path_ << ") "
                    << "SenderName: " << sender_name << " "
                    << "SignalName: " << signal_name << " "
                    << "ParameterType: " << g_variant_get_type_string(parameters);

  for (ReplyCallback callback: handlers_[signal_name])
    callback(parameters);
}

void DBusProxy::Impl::Call(string const& method_name,
                           GVariant* parameters,
                           ReplyCallback callback,
                           GCancellable* cancellable,
                           GDBusCallFlags flags,
                           int timeout_msec)
{
  if (proxy_)
  {
    CallData* data = new CallData();
    data->callback = callback;
    data->impl = this;
    data->method_name = method_name;

    g_dbus_proxy_call(proxy_,
                      method_name.c_str(),
                      parameters,
                      flags,
                      timeout_msec,
                      cancellable != NULL ? cancellable : cancellable_,
                      DBusProxy::Impl::OnCallCallback,
                      data);
  }
  else
  {
    LOG_WARNING(logger) << "Cannot call method " << method_name
                        << " proxy " << object_path_ << " does not exist";
  }
}

void DBusProxy::Impl::OnCallCallback(GObject* source, GAsyncResult* res, gpointer call_data)
{
  glib::Error error;
  std::unique_ptr<CallData> data (static_cast<CallData*>(call_data));
  GVariant* result = g_dbus_proxy_call_finish(G_DBUS_PROXY(source), res, &error);

  if (error && g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
  {
    // silently ignore
  }
  else if (error)
  {
    // Do not touch the impl pointer as the operation may have been cancelled
    LOG_WARNING(logger) << "Calling method \"" << data->method_name
      << "\" on object path: \""
      << g_dbus_proxy_get_object_path (G_DBUS_PROXY (source))
      << "\" failed: " << error;
  }
  else
  {
    data->callback(result);
    g_variant_unref(result);
  }
}

void DBusProxy::Impl::Connect(std::string const& signal_name, ReplyCallback callback)
{
  handlers_[signal_name].push_back(callback);
}

DBusProxy::DBusProxy(string const& name,
                     string const& object_path,
                     string const& interface_name,
                     GBusType bus_type,
                     GDBusProxyFlags flags)
  : pimpl(new Impl(this, name, object_path, interface_name, bus_type, flags))
{}

DBusProxy::~DBusProxy()
{
  delete pimpl;
}

void DBusProxy::Call(string const& method_name,
                     GVariant* parameters,
                     ReplyCallback callback,
                     GCancellable* cancellable,
                     GDBusCallFlags flags,
                     int timeout_msec)
{
  pimpl->Call(method_name, parameters, callback, cancellable, flags,
              timeout_msec);
}

void DBusProxy::Connect(std::string const& signal_name, ReplyCallback callback)
{
  pimpl->Connect(signal_name, callback);
}

bool DBusProxy::IsConnected()
{
  return pimpl->IsConnected();
}

}
}
