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

class DBusObject : public sigc::trackable
{
public:
  typedef std::shared_ptr<DBusObject> Ptr;

  DBusObject(std::string const& introspection_xml, std::string const& interface_name);
  virtual ~DBusObject();

  typedef std::function<GVariant*(std::string const&, GVariant*)> MethodCallback;
  typedef std::function<GVariant*(std::string const&)> PropertyGetterCallback;
  typedef std::function<bool(std::string const&, GVariant*)> PropertySetterCallback;

  void SetMethodsCallsHandler(MethodCallback const&);
  void SetPropertyGetter(PropertyGetterCallback const&);
  void SetPropertySetter(PropertySetterCallback const&);

  std::string InterfaceName() const;

  bool Register(glib::Object<GDBusConnection> const&, std::string const& path);
  void UnRegister(std::string const& path = "");

  void EmitSignal(std::string const& signal, GVariant* parameters = nullptr, std::string const& path = "");
  void EmitPropertyChanged(std::string const& property, std::string const& path = "");

  sigc::signal<void, std::string const&> registered;
  sigc::signal<void, std::string const&> unregistered;
  sigc::signal<void> fully_unregistered;

private:
  // not copyable class
  DBusObject(DBusObject const&) = delete;
  DBusObject& operator=(DBusObject const&) = delete;

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

class DBusObjectBuilder
{
public:
  static std::list<DBusObject::Ptr> GetObjectsForIntrospection(std::string const& introspection_xml);
};

class DBusServer : public sigc::trackable
{
public:
  typedef std::shared_ptr<DBusServer> Ptr;

  DBusServer(GBusType bus_type = G_BUS_TYPE_SESSION);
  DBusServer(std::string const& name, GBusType bus_type = G_BUS_TYPE_SESSION);
  virtual ~DBusServer();

  void AddObjects(std::string const& introspection_xml, std::string const& path);
  bool AddObject(DBusObject::Ptr const&, std::string const& path);
  bool RemoveObject(DBusObject::Ptr const&);

  std::list<DBusObject::Ptr> GetObjects() const;
  DBusObject::Ptr GetObject(std::string const& interface) const;

  void EmitSignal(std::string const& interface, std::string const& signal, GVariant* parameters = nullptr);

  bool IsConnected() const;
  std::string const& Name() const;
  bool OwnsName() const;

  sigc::signal<void> connected;
  sigc::signal<void> name_acquired;
  sigc::signal<void> name_lost;

private:
  // not copyable class
  DBusServer(DBusServer const&) = delete;
  DBusServer& operator=(DBusServer const&) = delete;

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace glib
} // namespace unity

#endif //UNITY_GLIB_DBUS_SERVER_H
