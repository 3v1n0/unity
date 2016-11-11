// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2015 Canonical Ltd
*               2015, National University of Defense Technology(NUDT) & Kylin Ltd
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
*              handsome_feng <jianfengli@ubuntukylin.com>
*/

#ifndef UNITY_KYLIN_LOCKSCREEN_SHIELD_H
#define UNITY_KYLIN_LOCKSCREEN_SHIELD_H

#include <UnityCore/ConnectionManager.h>
#include <UnityCore/GLibSource.h>
#include "LockScreenBaseShield.h"

namespace unity
{
namespace lockscreen
{

class AbstractUserPromptView;

class KylinShield : public BaseShield
{
public:
  KylinShield(session::Manager::Ptr const&,
         Accelerators::Ptr const&,
         nux::ObjectPtr<AbstractUserPromptView> const&,
         int monitor, bool is_primary);

private:
  void ShowPrimaryView() override;
};

}
}

#endif
