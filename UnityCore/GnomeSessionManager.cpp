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

#include "GnomeSessionManagerImpl.h"

namespace unity
{
namespace session
{

// Private implementation

GnomeManager::Impl::Impl()
  : can_shutdown_(true)
  , can_suspend_(false)
  , can_hibernate_(false)
  , upower_proxy_("org.freedesktop.UPower", "/org/freedesktop/UPower",
                  "org.freedesktop.UPower", G_BUS_TYPE_SYSTEM)
  , gsession_proxy_("org.gnome.SessionManager", "/org/gnome/SessionManager",
                    "org.gnome.SessionManager")
{
  upower_proxy_.Connect("Changed", sigc::hide(sigc::mem_fun(this, &GnomeManager::Impl::QueryUPowerCapabilities)));

  QueryUPowerCapabilities();

  gsession_proxy_.Call("CanShutdown", nullptr, [this] (GVariant* value) {
    can_shutdown_ = value ? g_variant_get_boolean(value) != FALSE : false;
  });
}

void GnomeManager::Impl::QueryUPowerCapabilities()
{
  upower_proxy_.Call("HibernateAllowed", nullptr, [this] (GVariant* value) {
    can_hibernate_ = value ? g_variant_get_boolean(value) != FALSE : false;
  });

  upower_proxy_.Call("SuspendAllowed", nullptr, [this] (GVariant* value) {
    can_suspend_ = value ? g_variant_get_boolean(value) != FALSE : false;
  });
}

// Public implementation

GnomeManager::GnomeManager()
  : impl_(new GnomeManager::Impl())
{}

void GnomeManager::Logout()
{
  enum LogoutMethods
  {
    NORMAL = 0,
    NO_CONFIRMATION,
    FORCE_LOGOUT
  };

  unsigned mode = LogoutMethods::NO_CONFIRMATION;
  impl_->gsession_proxy_.Call("Logout", g_variant_new_uint32(mode));
}

void GnomeManager::Reboot()
{
  impl_->gsession_proxy_.Call("RequestReboot");
}

void GnomeManager::Shutdown()
{
  impl_->gsession_proxy_.Call("RequestShutdown");
}

void GnomeManager::Suspend()
{
  impl_->upower_proxy_.Call("Suspend");
}

void GnomeManager::Hibernate()
{
  impl_->upower_proxy_.Call("Hibernate");
}

void GnomeManager::ConfirmLogout()
{
}

void GnomeManager::ConfirmReboot()
{
}

void GnomeManager::ConfirmShutdown()
{
}

void GnomeManager::CancelAction()
{
}

void GnomeManager::ClosedDialog()
{
}

bool GnomeManager::CanShutdown()
{
  return impl_->can_shutdown_;
}

bool GnomeManager::CanSuspend()
{
  return impl_->can_suspend_;
}

bool GnomeManager::CanHibernate()
{
  return impl_->can_hibernate_;
}

} // namespace session
} // namespace unity