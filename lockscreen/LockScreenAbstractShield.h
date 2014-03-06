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

#ifndef UNITY_LOCKSCREEN_ABSTRACT_SHIELD_H
#define UNITY_LOCKSCREEN_ABSTRACT_SHIELD_H

#include <NuxCore/Property.h>
#include <UnityCore/SessionManager.h>

#include "unity-shared/MockableBaseWindow.h"

namespace unity
{
namespace lockscreen
{

class AbstractShield : public MockableBaseWindow
{
public:
  AbstractShield(session::Manager::Ptr const& session, int monitor, bool is_primary)
    : MockableBaseWindow("Unity Lockscreen")
    , primary(is_primary)
    , monitor_(monitor)
    , session_manager_(session)
  {}

  nux::Property<bool> primary;

  sigc::signal<void, int, int> grab_motion;

protected:
  int monitor_;
  session::Manager::Ptr session_manager_;
};

} // lockscreen
} // unity

#endif // UNITY_LOCKSCREEN_ABSTRACT_SHIELD_H
