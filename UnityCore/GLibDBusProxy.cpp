// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011-2013 Canonical Ltd
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
 *              Michal Hruby <michal.hruby@canonical.com>
 *              Marco Trevisan <marco.trevisan@canonical.com>
 */

#include "GLibDBusProxy.h"

#include <map>
#include <memory>
#include <NuxCore/Logger.h>
#include <vector>

#include "GLibWrapper.h"
#include "GLibSignal.h"
#include "GLibSource.h"

namespace unity
{
namespace glib
{
DECLARE_LOGGER(logger, "unity.glib.dbus.proxy");

namespace
{
const unsigned MAX_RECONNECTION_ATTEMPTS = 5;
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

  void StartReconnectionTimeout(unsigned timeout);
  void Connect();

  void WaitForProxy(GCancellable* cancellable,
                    int timeout_msec,
                    std::function<void(glib::Error const&)> const& callback);
  void CallNoError(string const& method_name,
                   GVariant* parameters,
                   ReplyCallback const& callback,
                   GCancellable* cancellable,
                   GDBusCallFlags flags,
                   int timeout_msec);
  void Call(std::string const& method_name,
            GVariant* parameters,
            CallFinishedCallback const& callback,
            GCancellable *cancellable,
            GDBusCallFlags flags,
            int timeout_msec);

  void Connect(string const& signal_name, ReplyCallback const& callback);
  void DisconnectSignal(string const& signal_name);
  bool IsConnected() const;

  void OnProxyNameOwnerChanged(GDBusProxy*, GParamSpec*);
  void OnProxySignal(GDBusProxy* proxy, const char*, const char*, GVariant*);
  void OnPropertyChanged(GDBusProxy*, GVariant*, GStrv*);

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
  glib::Cancellable cancellable_;
  bool connected_;
  unsigned reconnection_attempts_;

  glib::Signal<void, GDBusProxy*, const char*, const char*, GVariant*> g_signal_connection_;
  glib::Signal<void, GDBusProxy*, GVariant*, GStrv*> g_property_signal_;
  glib::Signal<void, GDBusProxy*, GParamSpec*> name_owner_signal_;
  glib::Source::UniquePtr reconnect_timeout_;
  sigc::signal<void> proxy_acquired;

  SignalHandlers signal_handlers_;
  SignalHandlers property_handlers_;
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
  , connected_(false)
  , reconnection_attempts_(0)
{
  Connect();
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

bool DBusProxy::Impl::IsConnected() const
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
  self->name_owner_signal_.Connect(self->proxy_, "notify::g-name-owner",
                                   sigc::mem_fun(self, &Impl::OnProxyNameOwnerChanged));

  if (!self->signal_handlers_.empty())
  {
    // Connecting to the signals only if we have handlers
    if (glib::object_cast<GObject>(self->proxy_) != self->g_signal_connection_.object())
      self->g_signal_connection_.Connect(self->proxy_, "g-signal", sigc::mem_fun(self, &Impl::OnProxySignal));
  }

  if (!self->property_handlers_.empty())
  {
    // Connecting to the property-changed only if we have handlers
    if (glib::object_cast<GObject>(self->proxy_) != self->g_property_signal_.object())
      self->g_property_signal_.Connect(self->proxy_, "g-properties-changed", sigc::mem_fun(self, &Impl::OnPropertyChanged));
  }

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

void DBusProxy::Impl::OnProxySignal(GDBusProxy* proxy, const char* sender_name, const char* signal_name, GVariant* parameters)
{
  LOG_DEBUG(logger) << "Signal Received for proxy (" << object_path_ << ") "
                    << "SenderName: " << sender_name << " "
                    << "SignalName: " << signal_name << " "
                    << "ParameterType: " << g_variant_get_type_string(parameters);

  auto handler_it = signal_handlers_.find(signal_name);

  if (handler_it != signal_handlers_.end())
  {
    for (ReplyCallback const& callback : handler_it->second)
      callback(parameters);
  }
}

void DBusProxy::Impl::OnPropertyChanged(GDBusProxy* proxy, GVariant* changed_props, GStrv* invalidated)
{
  LOG_DEBUG(logger) << "Properties changed for proxy (" << object_path_ << ")";

  if (g_variant_n_children(changed_props) > 0)
  {
    GVariantIter *iter;
    const gchar *property_name;
    GVariant *value;

    g_variant_get(changed_props, "a{sv}", &iter);
    while (g_variant_iter_loop(iter, "{&sv}", &property_name, &value))
    {
      LOG_DEBUG(logger) << "Property: '" << property_name << "': " << value;

      auto handler_it = property_handlers_.find(property_name);

      if (handler_it != property_handlers_.end())
      {
        for (ReplyCallback const& callback : handler_it->second)
          callback(value);
      }
    }

    g_variant_iter_free (iter);
  }
}

void DBusProxy::Impl::WaitForProxy(GCancellable* cancellable,
                                   int timeout_msec,
                                   std::function<void(glib::Error const&)> const& callback)
{
  if (!proxy_)
  {
    auto con = std::make_shared<sigc::connection>();
    auto canc = glib::Object<GCancellable>(cancellable, glib::AddRef());

    // add a timeout
    auto timeout = std::make_shared<glib::Timeout>(timeout_msec < 0 ? 30000 : timeout_msec, [con, canc, callback] ()
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
    });
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
                                  ReplyCallback const& callback,
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
                           CallFinishedCallback const& callback,
                           GCancellable* cancellable,
                           GDBusCallFlags flags,
                           int timeout_msec)
{
  GCancellable* target_canc = cancellable != NULL ? cancellable : cancellable_;

  if (!proxy_)
  {
    glib::Variant sinked_parameters(parameters);
    glib::Object<GCancellable>canc(target_canc, glib::AddRef());
    WaitForProxy(canc, timeout_msec, [this, method_name, sinked_parameters, callback, canc, flags, timeout_msec] (glib::Error const& err)
    {
      if (err)
      {
        callback(glib::Variant(), err);
        LOG_WARNING(logger) << "Cannot call method " << method_name
                            << ": " << err;
      }
      else
      {
        Call(method_name, sinked_parameters, callback, canc, flags, timeout_msec);
      }
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
                    target_canc,
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

void DBusProxy::Impl::Connect(std::string const& signal_name, ReplyCallback const& callback)
{
  if (!callback)
    return;

  if (proxy_ && glib::object_cast<GObject>(proxy_) != g_signal_connection_.object())
  {
    // It's the first time we connect to a signal so we need to setup the call handler
    g_signal_connection_.Connect(proxy_, "g-signal", sigc::mem_fun(this, &Impl::OnProxySignal));
  }

  signal_handlers_[signal_name].push_back(callback);
}

void DBusProxy::Impl::DisconnectSignal(std::string const& signal_name)
{
  if (signal_name.empty())
  {
    signal_handlers_.clear();
  }
  else
  {
    signal_handlers_.erase(signal_name);
  }

  if (signal_handlers_.empty())
    g_signal_connection_.Disconnect();
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
                     ReplyCallback const& callback,
                     GCancellable* cancellable,
                     GDBusCallFlags flags,
                     int timeout_msec)
{
  pimpl->CallNoError(method_name, parameters, callback, cancellable, flags,
                     timeout_msec);
}

void DBusProxy::CallBegin(std::string const& method_name,
                          GVariant* parameters,
                          CallFinishedCallback const& callback,
                          GCancellable *cancellable,
                          GDBusCallFlags flags,
                          int timeout_msec)
{
  pimpl->Call(method_name, parameters, callback, cancellable, flags,
              timeout_msec);
}

glib::Variant DBusProxy::GetProperty(std::string const& name) const
{
  if (IsConnected())
    return Variant(g_dbus_proxy_get_cached_property(pimpl->proxy_, name.c_str()), StealRef());

  return nullptr;
}

void DBusProxy::GetProperty(std::string const& name, ReplyCallback const& callback)
{
  if (!callback)
    return;

  if (IsConnected())
  {
    g_dbus_connection_call(g_dbus_proxy_get_connection(pimpl->proxy_),
                           pimpl->name_.c_str(), pimpl->object_path_.c_str(),
                           "org.freedesktop.DBus.Properties",
                           "Get", g_variant_new ("(ss)", pimpl->interface_name_.c_str(), name.c_str()),
                            G_VARIANT_TYPE("(v)"), G_DBUS_CALL_FLAGS_NONE, -1, pimpl->cancellable_,
                           [] (GObject *source, GAsyncResult *res, gpointer user_data) {
      glib::Error err;
      std::unique_ptr<ReplyCallback> callback(static_cast<ReplyCallback*>(user_data));
      Variant result(g_dbus_connection_call_finish(G_DBUS_CONNECTION(source), res, &err));

      if (err)
      {
        LOG_ERROR(logger) << "Impossible to get property: " << err;
        return;
      }

      Variant value;
      g_variant_get(result, "(v)", &value);

      (*callback)(value);
    }, new ReplyCallback(callback));
  }
  else
  {
    // This will get the property as soon as we have a connection
    auto conn = std::make_shared<sigc::connection>();
    *conn = connected.connect([this, conn, name, callback] {
      GetProperty(name, callback);
      conn->disconnect();
    });
  }
}

void DBusProxy::SetProperty(std::string const& name, GVariant* value)
{
  if (IsConnected())
  {
    g_dbus_connection_call(g_dbus_proxy_get_connection(pimpl->proxy_),
                           pimpl->name_.c_str(), pimpl->object_path_.c_str(),
                           "org.freedesktop.DBus.Properties",
                           "Set", g_variant_new ("(ssv)", pimpl->interface_name_.c_str(), name.c_str(), value),
                           nullptr, G_DBUS_CALL_FLAGS_NONE, -1, pimpl->cancellable_,
                           [] (GObject *source, GAsyncResult *res, gpointer user_data) {
      glib::Error err;
      Variant result(g_dbus_connection_call_finish(G_DBUS_CONNECTION(source), res, &err));
      if (err)
      {
        LOG_ERROR(logger) << "Impossible to set property: " << err;
      }
    }, this);
  }
  else
  {
    // This will set the property as soon as we have a connection
    auto conn = std::make_shared<sigc::connection>();
    *conn = connected.connect([this, conn, name, value] {
      SetProperty(name, value);
      conn->disconnect();
    });
  }
}

void DBusProxy::ConnectProperty(std::string const& name, ReplyCallback const& callback)
{
  if (!callback || name.empty())
  {
    LOG_WARN(logger) << "Impossible to connect to empty property or with invalid callback";
    return;
  }

  if (pimpl->proxy_ && glib::object_cast<GObject>(pimpl->proxy_) != pimpl->g_property_signal_.object())
  {
    // It's the first time we connect to a property so we need to setup the call handler
    pimpl->g_property_signal_.Connect(pimpl->proxy_, "g-properties-changed", sigc::mem_fun(pimpl.get(), &Impl::OnPropertyChanged));
  }

  pimpl->property_handlers_[name].push_back(callback);
}

void DBusProxy::DisconnectProperty(std::string const& name)
{
  if (name.empty())
  {
    pimpl->property_handlers_.clear();
  }
  else
  {
    pimpl->property_handlers_.erase(name);
  }

  if (pimpl->property_handlers_.empty())
    pimpl->g_property_signal_.Disconnect();
}

void DBusProxy::Connect(std::string const& signal_name, ReplyCallback const& callback)
{
  pimpl->Connect(signal_name, callback);
}

void DBusProxy::DisconnectSignal(std::string const& signal_name)
{
  pimpl->DisconnectSignal(signal_name);
}

bool DBusProxy::IsConnected() const
{
  return pimpl->IsConnected();
}

}
}
