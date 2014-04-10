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
#include <UnityCore/Indicators.h>

#include "unity-shared/MockableBaseWindow.h"

namespace unity
{
namespace lockscreen
{

class AbstractShield : public MockableBaseWindow
{
public:
  AbstractShield(session::Manager::Ptr const& session, indicator::Indicators::Ptr const& indicators, int monitor_num, bool is_primary)
    : MockableBaseWindow("Unity Lockscreen")
    , primary(is_primary)
    , monitor(monitor_num)
    , session_manager_(session)
    , indicators_(indicators)
  {}

  nux::Property<bool> primary;
  nux::Property<int> monitor;

  using MockableBaseWindow::RemoveLayout;
  virtual bool IsIndicatorOpen() const = 0;
  virtual void CheckCapsLockPrompt() = 0;

  sigc::signal<void, int, int> grab_motion;
  sigc::signal<void, unsigned long, unsigned long> grab_key;

protected:
  session::Manager::Ptr session_manager_;
  indicator::Indicators::Ptr indicators_;
};

} // lockscreen
} // unity

#endif // UNITY_LOCKSCREEN_ABSTRACT_SHIELD_H
