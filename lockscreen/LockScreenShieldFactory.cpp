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

#include "LockScreenShieldFactory.h"
#include "LockScreenShield.h"

namespace unity
{
namespace lockscreen
{

nux::ObjectPtr<MockableBaseWindow> ShieldFactory::CreateShield(session::Manager::Ptr const& session_manager,
                                                               bool is_primary)
{
  nux::ObjectPtr<Shield> shield(new Shield(session_manager, is_primary));
  return shield;
}

}
}
