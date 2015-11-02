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
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 */

#ifndef UNITYSHELL_SCREENSAVER_DBUS_MANAGER_H
#define UNITYSHELL_SCREENSAVER_DBUS_MANAGER_H

#include <NuxCore/Property.h>
#include <UnityCore/GLibDBusServer.h>
#include <UnityCore/SessionManager.h>

namespace unity
{
namespace lockscreen
{

class DBusManager
{
public:
  typedef std::shared_ptr<DBusManager> Ptr;

  DBusManager(session::Manager::Ptr const&);
  ~DBusManager();

  nux::Property<bool> active;

  sigc::signal<void> simulate_activity;

protected:
  struct TestMode {};
  DBusManager(session::Manager::Ptr const&, TestMode const&);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  friend struct TestScreenSaverDBusManager;
};

} // lockscreen
} // unity

#endif
