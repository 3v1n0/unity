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

#include <NuxCore/Property.h>

namespace unity
{
namespace session
{

class Manager
{
public:
  typedef std::shared_ptr<Manager> Ptr;

  Manager() = default;
  virtual ~Manager() = default;

  nux::ROProperty<bool> have_other_open_sessions;
  nux::Property<bool> is_locked;
  nux::ROProperty<bool> is_session_active;

  virtual std::string RealName() const = 0;
  virtual std::string UserName() const = 0;
  virtual std::string HostName() const = 0;
  virtual void UserIconFile(std::function<void(std::string const&)> const&) const = 0;
  virtual bool AutomaticLogin() const = 0;

  virtual void ScreenSaverActivate() = 0;
  virtual void ScreenSaverDeactivate() = 0;
  virtual void LockScreen() = 0;
  virtual void PromptLockScreen() = 0;
  virtual void Logout() = 0;
  virtual void Reboot() = 0;
  virtual void Shutdown() = 0;
  virtual void Suspend() = 0;
  virtual void Hibernate() = 0;
  virtual void SwitchToGreeter() = 0;

  virtual bool CanLock() const = 0;
  virtual bool CanShutdown() const = 0;
  virtual bool CanSuspend() const = 0;
  virtual bool CanHibernate() const = 0;
  virtual bool HasInhibitors() const = 0;

  virtual void CancelAction() = 0;

  // not copyable class
  Manager(const Manager&) = delete;
  Manager& operator=(const Manager&) = delete;

  sigc::signal<void> lock_requested;
  sigc::signal<void> unlock_requested;
  sigc::signal<void> prompt_lock_requested;
  sigc::signal<void> locked;
  sigc::signal<void> unlocked;
  sigc::signal<void, bool /* inhibitors */> logout_requested;
  sigc::signal<void, bool /* inhibitors */> reboot_requested;
  sigc::signal<void, bool /* inhibitors */> shutdown_requested;
  sigc::signal<void, bool /* is_idle */> presence_status_changed;
  sigc::signal<void, bool /* active */> screensaver_requested;

  sigc::signal<void> cancel_requested;
};

} // namespace session
} // namespace unity

#endif //UNITY_SESSION_MANAGER_H
