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

#include <NuxCore/Logger.h>

#include "GLibWrapper.h"
#include "GLibSignal.h"

namespace unity {
namespace glib {

namespace {
nux::logging::Logger logger("unity.glib.dbusproxy");
}

class DBusProxy::Impl
{
public:

  Impl(DBusProxy* owner,
       std::string const& name,
       std::string const& object_path,
       std::string const& interface_name,
       GBusType bus_type,
       GDBusProxyFlags flags);
  ~Impl();

  static void OnNameAppeared(GDBusConnection* connection, const char* name, const char* name_owner, gpointer impl);
  static void OnNameVanished(GDBusConnection* connection, const char* name, gpointer impl);
  void StartReconnectionTimeout();
  void Connect();
  static void OnProxyConnectCallback(GObject* source, GAsyncResult* res, gpointer impl);

  void OnProxySignal(GDBusProxy* proxy, char* sender_name, char* signal_name, GVariant* parameters);

  DBusProxy* owner_;
  std::string name_;
  std::string object_path_;
  std::string interface_name_;
  GBusType bus_type_;
  GDBusProxyFlags flags_;

  glib::Object<GDBusProxy> proxy_;
  glib::Object<GCancellable> cancellable_;
  guint watcher_id_;
  guint reconnect_timeout_id_;
  bool connected_;

  glib::Signal<void, GDBusProxy*, char*, char*, GVariant*> g_signal_connection_;

};

DBusProxy::Impl::Impl(DBusProxy* owner,
                      std::string const& name,
                      std::string const& object_path,
                      std::string const& interface_name,
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
    g_source_remove (reconnect_timeout_id_);
}

void DBusProxy::Impl::OnNameAppeared(GDBusConnection* connection,
                                     const char* name,
                                     const char* name_owner,
                                     gpointer impl)
{
  DBusProxy::Impl *self = reinterpret_cast<DBusProxy::Impl*>(impl);

  LOG_DEBUG(logger) << self->name_ << " appeared";

  self->Connect();
}

void DBusProxy::Impl::OnNameVanished(GDBusConnection* connection,
                                     const char* name,
                                     gpointer impl)
{
  DBusProxy::Impl *self = reinterpret_cast<DBusProxy::Impl*>(impl);
  
  LOG_DEBUG(logger) << self->name_ << " vanished";
  
  self->proxy_ = NULL;
  self->g_signal_connection_.Disconnect();
  self->connected_ = false;
  self->owner_->disconnected.emit();
}

void DBusProxy::Impl::StartReconnectionTimeout()
{
  LOG_DEBUG(logger) << "Starting reconnection timeout for " << name_;
  if (reconnect_timeout_id_)
    g_source_remove(reconnect_timeout_id_);

  auto callback = [] (gpointer user_data) -> gboolean
  {
    DBusProxy::Impl* self = reinterpret_cast<DBusProxy::Impl*>(user_data);
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

void DBusProxy::Impl::OnProxyConnectCallback(GObject* source,
                                             GAsyncResult* res,
                                             gpointer impl)
{
  glib::Error error;
  glib::Object<GDBusProxy> proxy(g_dbus_proxy_new_for_bus_finish(res, error.AsOutParam()));

  // If the async operation was cancelled, this callback will still be called and
  // therefore we should deal with the error before touching the impl pointer
  if (!proxy || error)
  {
    LOG_WARNING(logger) << "Unable to connect to proxy: " << error.Message();
    return;
  }

  DBusProxy::Impl *self = reinterpret_cast<DBusProxy::Impl*>(impl);
  self->proxy_ = proxy;
  self->g_signal_connection_.Connect(self->proxy_, "g-signal", sigc::mem_fun(self, &DBusProxy::Impl::OnProxySignal));
  self->connected_ = true;
  self->owner_->connected.emit();
}

void DBusProxy::Impl::OnProxySignal(GDBusProxy* proxy,
                                    char* sender_name,
                                    char* signal_name,
                                    GVariant* parameters)
{
  g_debug ("SIGNAL: %s %s", sender_name, signal_name);
}

DBusProxy::DBusProxy(std::string const& name,
                     std::string const& object_path,
                     std::string const& interface_name,
                     GBusType bus_type,
                     GDBusProxyFlags flags)
  : pimpl(new Impl(this, name, object_path, interface_name, bus_type, flags))
{}

DBusProxy::~DBusProxy()
{
  delete pimpl;
}

}
}
