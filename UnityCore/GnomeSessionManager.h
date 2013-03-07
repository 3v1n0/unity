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

#ifndef UNITY_GNOME_SESSION_MANAGER_H
#define UNITY_GNOME_SESSION_MANAGER_H

#include "SessionManager.h"

namespace unity
{
namespace session
{

class GnomeManager : public Manager
{
public:
  GnomeManager();
  ~GnomeManager();

  std::string RealName() const;
  std::string UserName() const;

  void LockScreen();
  void Logout();
  void Reboot();
  void Shutdown();
  void Suspend();
  void Hibernate();

  bool CanShutdown() const;
  bool CanSuspend() const;
  bool CanHibernate() const;

  void CancelAction();

  struct Impl;
protected:
  struct TestMode {};
  GnomeManager(TestMode const&);

private:
  std::unique_ptr<Impl> impl_;
};

} // namespace session
} // namespace unity

#endif //UNITY_GNOME_SESSION_MANAGER_H
