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

namespace session
{

GnomeManager::GnomeManager()
{

}

void GnomeManager::Logout()
{

}

void GnomeManager::Reboot()
{

}

void GnomeManager::Shutdown()
{

}

void GnomeManager::Suspend()
{

}

void GnomeManager::Hibernate()
{

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
  return false;
}

bool GnomeManager::CanSuspend()
{
  return false;
}

bool GnomeManager::CanHibernate()
{
  return false;
}

} // namespace session