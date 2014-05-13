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

#ifndef UNITY_LOCKSCREEN_SHIELD_FACTORY
#define UNITY_LOCKSCREEN_SHIELD_FACTORY

#include <Nux/Nux.h>
#include "LockScreenAbstractShield.h"

namespace unity
{

class MockableBaseWindow;

namespace lockscreen
{

struct ShieldFactoryInterface
{
  typedef std::shared_ptr<ShieldFactoryInterface> Ptr;

  virtual ~ShieldFactoryInterface() = default;

  virtual nux::ObjectPtr<AbstractShield> CreateShield(session::Manager::Ptr const&, indicator::Indicators::Ptr const&, Accelerators::Ptr const&, int monitor, bool is_primary) = 0;
};

struct ShieldFactory : ShieldFactoryInterface
{
  nux::ObjectPtr<AbstractShield> CreateShield(session::Manager::Ptr const&, indicator::Indicators::Ptr const&, Accelerators::Ptr const&, int monitor, bool is_primary) override;
};

}
}

#endif
