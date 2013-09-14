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

#include <gmock/gmock.h>
#include <UnityCore/SessionManager.h>

namespace unity
{
namespace session
{

struct MockManager : Manager
{
  typedef std::shared_ptr<MockManager> Ptr;

  MOCK_CONST_METHOD0(RealName, std::string());
  MOCK_CONST_METHOD0(UserName, std::string());

  MOCK_METHOD0(LockScreen, void());
  MOCK_METHOD0(Logout, void());
  MOCK_METHOD0(Reboot, void());
  MOCK_METHOD0(Shutdown, void());
  MOCK_METHOD0(Suspend, void());
  MOCK_METHOD0(Hibernate, void());
  MOCK_METHOD0(CancelAction, void());

  MOCK_CONST_METHOD0(CanShutdown, bool());
  MOCK_CONST_METHOD0(CanSuspend, bool());
  MOCK_CONST_METHOD0(CanHibernate, bool());
};

} // session
} // unity