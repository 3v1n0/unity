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
#include "KylinLockScreenShield.h"
#include "unity-shared/UnitySettings.h"

namespace unity
{
namespace lockscreen
{

nux::ObjectPtr<BaseShield> ShieldFactory::CreateShield(session::Manager::Ptr const& session_manager,
                                                       menu::Manager::Ptr const& menu_manager,
                                                       Accelerators::Ptr const& accelerators,
                                                       nux::ObjectPtr<AbstractUserPromptView> const& prompt_view,
                                                       int monitor, bool is_primary)
{
  nux::ObjectPtr<BaseShield> shield;

  if (Settings::Instance().desktop_type() == DesktopType::UBUNTUKYLIN)
    shield = new KylinShield(session_manager, accelerators, prompt_view, monitor, is_primary);
  else
    shield = new Shield(session_manager, menu_manager, accelerators, prompt_view,  monitor, is_primary);

  return shield;
}

}
}
