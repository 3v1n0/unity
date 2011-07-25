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

#ifndef UNITY_DBUS_LENSES_H
#define UNITY_DBUS_LENSES_H

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <gio/gio.h>
#include <sigc++/signal.h>
#include <sigc++/trackable.h>

namespace unity
{
namespace glib
{

// A light wrapper around GDBusProxy which allows for some nicer signal and async
// method usage from C++
class DBusProxy : public sigc::trackable, boost::noncopyable
{
public:
  typedef boost::shared_ptr<DBusProxy> Ptr;
  typedef sigc::slot<void, GVariant*> ReplyCallback;

  DBusProxy(std::string const& name,
            std::string const& object_path,
            std::string const& interface_name,
            GBusType bus_type = G_BUS_TYPE_SESSION,
            GDBusProxyFlags flags = G_DBUS_PROXY_FLAGS_NONE);
  ~DBusProxy();

  void Call(std::string const& method_name,
            GVariant* parameters = NULL,
            ReplyCallback callback = sigc::ptr_fun(&NoReplyCallback),
            GDBusCallFlags flags = G_DBUS_CALL_FLAGS_NONE,
            int timeout_msec = -1);

  void Connect(std::string const& signal_name, ReplyCallback callback);

  sigc::signal<void> connected;
  sigc::signal<void> disconnected;

  static void NoReplyCallback(GVariant* v) {};
  class Impl;
private:
  Impl* pimpl;
};

}
}

#endif
