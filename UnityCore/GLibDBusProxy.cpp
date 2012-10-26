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
#include "GLibSource.h"
#include "Variant.h"

namespace unity
{
namespace glib
{

namespace
{
const unsigned MAX_RECONNECTION_ATTEMPTS = 5;
nux::logging::Logger logger("unity.glib.dbusproxy");
}

using std::string;

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

  void StartReconnectionTimeout(unsigned timeout);
  void Connect();

  void WaitForProxy(GCancellable* cancellable,
                    int timeout_msec,
                    std::function<void(glib::Error const&)> callback);
  void CallNoError(string const& method_name,
                   GVariant* parameters,
                   ReplyCallback callback,
                   GCancellable* cancellable,
                   GDBusCallFlags flags,
                   int timeout_msec);
  void Call(std::string const& method_name,
            GVariant* parameters,
            CallFinishedCallback callback,
            GCancellable *cancellable,
            GDBusCallFlags flags,
            int timeout_msec);

  void Connect(string const& signal_name, ReplyCallback callback);
  bool IsConnected();

  void OnProxyNameOwnerChanged(GDBusProxy*, GParamSpec*);
  void OnProxySignal(GDBusProxy* proxy, char* sender_name, char* signal_name,
                     GVariant* parameters);

  static void OnProxyConnectCallback(GObject* source, GAsyncResult* res, gpointer impl);
  static void OnCallCallback(GObject* source, GAsyncResult* res, gpointer call_data);

  struct CallData
  {
    DBusProxy::CallFinishedCallback callback;
    std::string method_name;
  };

  DBusProxy* owner_;
  string name_;
  string object_path_;
  string interface_name_;
  GBusType bus_type_;
  GDBusProxyFlags flags_;

  glib::Object<GDBusProxy> proxy_;
  glib::Object<GCancellable> cancellable_;
  bool connected_;
  unsigned reconnection_attempts_;

  glib::Signal<void, GDBusProxy*, char*, char*, GVariant*> g_signal_connection_;
  glib::Signal<void, GDBusProxy*, GParamSpec*> name_owner_signal_;
  glib::Source::UniquePtr reconnect_timeout_;
  sigc::signal<void> proxy_acquired;

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
  , connected_(false)
  , reconnection_attempts_(0)
{
  // FIXME: get rid of this once glib doesn't deadlock in class initiation
  StartReconnectionTimeout(1);
}

DBusProxy::Impl::~Impl()
{
  g_cancellable_cancel(cancellable_);
}

void DBusProxy::Impl::StartReconnectionTimeout(unsigned timeout)
{
  LOG_DEBUG(logger) << "Starting reconnection timeout for " << name_;

  auto callback = [&]
  {
    if (!proxy_)
      Connect();

    return false;
  };

  reconnect_timeout_.reset(new glib::TimeoutSeconds(timeout, callback));
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
  DBusProxy::Impl* self = static_cast<DBusProxy::Impl*>(impl);

  glib::Error error;
  glib::Object<GDBusProxy> proxy(g_dbus_proxy_new_for_bus_finish(res, &error));

  // If the async operation was cancelled, this callback will still be called and
  // therefore we should deal with the error before touching the impl pointer
  if (!proxy || error)
  {
    // if the cancellable was cancelled, "self" points to destroyed object
    if (error && !g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    {
      if (self->reconnection_attempts_++ < MAX_RECONNECTION_ATTEMPTS)
      {
        LOG_WARNING(logger) << "Unable to connect to proxy: \"" << error
          << "\"... Trying to reconnect (attempt "
          << self->reconnection_attempts_ << ")";
        self->StartReconnectionTimeout(3);
      }
      else
      {
        LOG_ERROR(logger) << "Unable to connect to proxy: " << error;
      }
    }
    return;
  }

  LOG_DEBUG(logger) << "Sucessfully created proxy: " << self->object_path_;

  self->reconnection_attempts_ = 0;
  self->proxy_ = proxy;
  self->g_signal_connection_.Connect(self->proxy_, "g-signal",
                                     sigc::mem_fun(self, &Impl::OnProxySignal));
  self->name_owner_signal_.Connect(self->proxy_, "notify::g-name-owner",
                                   sigc::mem_fun(self, &Impl::OnProxyNameOwnerChanged));

  // If a proxy cannot autostart a service, it doesn't throw an error, but
  // sets name_owner to NULL
  if (glib::String(g_dbus_proxy_get_name_owner(proxy)))
  {
    self->connected_ = true;
    self->owner_->connected.emit();
  }

  self->proxy_acquired.emit();
}

void DBusProxy::Impl::OnProxyNameOwnerChanged(GDBusProxy* proxy, GParamSpec* param)
{
  glib::String name_owner(g_dbus_proxy_get_name_owner(proxy));

  if (name_owner)
  {
    if (!connected_)
    {
      LOG_DEBUG(logger) << name_ << " appeared";

      connected_ = true;
      owner_->connected.emit();
    }
  }
  else if (connected_)
  {
    LOG_DEBUG(logger) << name_ << " vanished";

    connected_ = false;
    owner_->disconnected.emit();
  }
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

void DBusProxy::Impl::WaitForProxy(GCancellable* cancellable,
                                   int timeout_msec,
                                   std::function<void(glib::Error const&)> callback)
{
  if (!proxy_)
  {
    auto con = std::make_shared<sigc::connection>();
    auto canc = glib::Object<GCancellable>(cancellable, glib::AddRef());

    // add a timeout
    auto timeout = glib::Source::Ptr(new glib::Timeout(timeout_msec < 0 ? 30000 : timeout_msec, [con, canc, callback] ()
    {
      if (!g_cancellable_is_cancelled(canc))
      {
        glib::Error err;
        GError** real_err = &err;
        *real_err = g_error_new_literal(G_DBUS_ERROR, G_DBUS_ERROR_TIMED_OUT,
                                        "Timed out waiting for proxy");
        callback(err);
      }
      con->disconnect();
      return false;
    }));
    // wait for the signal
    *con = proxy_acquired.connect([con, canc, timeout, callback] ()
    {
      if (!g_cancellable_is_cancelled(canc)) callback(glib::Error());
      timeout->Remove();
      con->disconnect();
    });
  }
  else
  {
    callback(glib::Error());
  }
}

void DBusProxy::Impl::CallNoError(string const& method_name,
                                  GVariant* parameters,
                                  ReplyCallback callback,
                                  GCancellable* cancellable,
                                  GDBusCallFlags flags,
                                  int timeout_msec)
{
  if (callback)
  {
    auto cb = [callback] (GVariant* result, Error const& err)
    {
      if (!err) callback(result);
    };

    Call(method_name, parameters, cb, cancellable, flags, timeout_msec);
  }
  else
  {
    Call(method_name, parameters, nullptr, cancellable, flags, timeout_msec);
  }
}

void DBusProxy::Impl::Call(string const& method_name,
                           GVariant* parameters,
                           CallFinishedCallback callback,
                           GCancellable* cancellable,
                           GDBusCallFlags flags,
                           int timeout_msec)
{
  if (!proxy_)
  {
    glib::Variant sinked_parameters(parameters);
    glib::Object<GCancellable>canc(cancellable, glib::AddRef());
    WaitForProxy(canc, timeout_msec, [this, method_name, sinked_parameters, callback, canc, flags, timeout_msec] (glib::Error const& err)
    {
      if (err)
      {
        callback(glib::Variant(), err);
        LOG_WARNING(logger) << "Cannot call method " << method_name
                            << ": " << err;
      }
      else
        Call(method_name, sinked_parameters, callback, canc, flags, timeout_msec);
    });
    return;
  }

  CallData* data = new CallData();
  data->callback = callback;
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

void DBusProxy::Impl::OnCallCallback(GObject* source, GAsyncResult* res, gpointer call_data)
{
  glib::Error error;
  std::unique_ptr<CallData> data(static_cast<CallData*>(call_data));
  glib::Variant result(g_dbus_proxy_call_finish(G_DBUS_PROXY(source), res, &error), glib::StealRef());

  if (error)
  {
    if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    {
      // silently ignore, don't even invoke callback, FIXME: really?
      return;
    }
    else
    {
      LOG_WARNING(logger) << "Calling method \"" << data->method_name
        << "\" on object path: \""
        << g_dbus_proxy_get_object_path(G_DBUS_PROXY(source))
        << "\" failed: " << error;
    }
  }

  if (data->callback)
    data->callback(result, error);
}

void DBusProxy::Impl::Connect(std::string const& signal_name, ReplyCallback callback)
{
  if (callback)
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
{}

void DBusProxy::Call(string const& method_name,
                     GVariant* parameters,
                     ReplyCallback callback,
                     GCancellable* cancellable,
                     GDBusCallFlags flags,
                     int timeout_msec)
{
  pimpl->CallNoError(method_name, parameters, callback, cancellable, flags,
                     timeout_msec);
}

void DBusProxy::CallBegin(std::string const& method_name,
                          GVariant* parameters,
                          CallFinishedCallback callback,
                          GCancellable *cancellable,
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
