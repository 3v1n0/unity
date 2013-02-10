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
* Authored by: Marco Trevisan (Treviño) <marco@ubuntu.com>
*/

#ifndef UNITY_SESSION_MANAGER_H
#define UNITY_SESSION_MANAGER_H

#include <sigc++/sigc++.h>
#include <memory>

namespace session
{

class Manager
{
public:
  typedef std::shared_ptr<Manager> Ptr;

  Manager() {}
  virtual ~Manager() = default;

  virtual void Logout() = 0;
  virtual void Reboot() = 0;
  virtual void Shutdown() = 0;
  virtual void Suspend() = 0;
  virtual void Hibernate() = 0;

  virtual void ConfirmLogout() = 0;
  virtual void ConfirmReboot() = 0;
  virtual void ConfirmShutdown() = 0;
  virtual void CancelAction() = 0;
  virtual void ClosedDialog() = 0;

  virtual bool CanShutdown() = 0;
  virtual bool CanSuspend() = 0;
  virtual bool CanHibernate() = 0;

  // not copyable class
  Manager(const Manager&) = delete;
  Manager& operator=(const Manager&) = delete;

  sigc::signal<void, unsigned> logout_requested;
  sigc::signal<void, unsigned> reboot_requested;
  sigc::signal<void, unsigned> shutdown_requested;
};

} // namespace session

#endif //UNITY_SESSION_MANAGER_H