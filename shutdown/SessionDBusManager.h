// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2014 Canonical Ltd
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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#ifndef UNITYSHELL_SESSION_DBUS_MANAGER_H
#define UNITYSHELL_SESSION_DBUS_MANAGER_H

#include "UnityCore/ConnectionManager.h"
#include "UnityCore/GLibDBusServer.h"
#include "UnityCore/SessionManager.h"

namespace unity
{
namespace session
{

class DBusManager
{
public:
  typedef std::shared_ptr<DBusManager> Ptr;

  DBusManager(session::Manager::Ptr const& manager);
  virtual ~DBusManager() = default;

private:
  session::Manager::Ptr session_;
  glib::DBusServer server_;
  glib::DBusObject::Ptr object_;
  connection::Manager connections_;
};

} // session
} // unity

#endif
