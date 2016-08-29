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
* Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
*/

#ifndef UNITY_LOCKSCREEN_SHIELD_H
#define UNITY_LOCKSCREEN_SHIELD_H

#include "LockScreenBaseShield.h"
#include "unity-shared/MenuManager.h"


namespace unity
{
namespace lockscreen
{

class AbstractUserPromptView;
class Panel;

class Shield : public BaseShield
{
public:
  Shield(session::Manager::Ptr const&,
         menu::Manager::Ptr const&,
         Accelerators::Ptr const&,
         nux::ObjectPtr<AbstractUserPromptView> const&,
         int monitor, bool is_primary);

  bool IsIndicatorOpen() const override;
  void ActivatePanel() override;

protected:
  nux::Area* FindKeyFocusArea(unsigned int, unsigned long, unsigned long) override;

private:
  void ShowPrimaryView() override;
  Panel* CreatePanel();

  menu::Manager::Ptr menu_manager_;
  Panel* panel_view_;
};

}
}

#endif
