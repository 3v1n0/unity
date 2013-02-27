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

#ifndef UNITY_GLIB_DBUS_SERVER_H
#define UNITY_GLIB_DBUS_SERVER_H

#include <gio/gio.h>
#include <memory>
#include <sigc++/signal.h>
#include <sigc++/trackable.h>

#include "GLibWrapper.h"

namespace unity
{
namespace glib
{

class DBusObject
{
public:
  typedef std::shared_ptr<DBusObject> Ptr;

  DBusObject(std::string const& introspection_xml, std::string const& interface_name);
  virtual ~DBusObject();

  typedef std::function<GVariant*(std::string const&, GVariant*)> MethodCallback;
  typedef std::function<GVariant*(std::string const&)> PropertyGetterCallback;
  typedef std::function<bool(std::string const&, GVariant*)> PropertySetterCallback;

  void SetMethodsCallHandler(MethodCallback const&);
  void SetPropertyGetter(PropertyGetterCallback const&);
  void SetPropertySetter(PropertySetterCallback const&);

  std::string InterfaceName() const;

  bool Register(glib::Object<GDBusConnection> const&, std::string const& path);

private:
  // not copyable class
  DBusObject(DBusObject const&) = delete;
  DBusObject& operator=(DBusObject const&) = delete;

  MethodCallback method_cb_;
  PropertyGetterCallback property_get_cb_;
  PropertySetterCallback property_set_cb_;

  std::shared_ptr<GDBusInterfaceInfo> interface_info_;
  GDBusInterfaceVTable interface_vtable_;
  std::map<guint, glib::Object<GDBusConnection>> registrations_;
};

class DBusServer : public sigc::trackable
{
public:
  typedef std::shared_ptr<DBusServer> Ptr;

  DBusServer(std::string const& name, GBusType bus_type = G_BUS_TYPE_SESSION);
  virtual ~DBusServer();

  bool AddObject(DBusObject::Ptr const&, std::string const& path);
  bool OwnsName() const;

  sigc::signal<void> name_acquired;
  sigc::signal<void> name_lost;

private:
  // not copyable class
  DBusServer(DBusServer const&) = delete;
  DBusServer& operator=(DBusServer const&) = delete;

  glib::Object<GDBusConnection> connection_;
  guint owner_name_;
  bool name_owned_;
  std::vector<DBusObject::Ptr> objects_;
};

} // namespace glib
} // namespace unity

#endif //UNITY_GLIB_DBUS_SERVER_H
