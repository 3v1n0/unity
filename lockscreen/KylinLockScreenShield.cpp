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

#include "KylinLockScreenShield.h"

#include <Nux/VLayout.h>
#include <Nux/HLayout.h>

#include "CofView.h"
#include "LockScreenSettings.h"
#include "LockScreenAbstractPromptView.h"

namespace unity
{
namespace lockscreen
{

KylinShield::KylinShield(session::Manager::Ptr const& session_manager,
               Accelerators::Ptr const& accelerators,
               nux::ObjectPtr<AbstractUserPromptView> const& prompt_view,
               int monitor_num, bool is_primary)
  : BaseShield(session_manager, accelerators, prompt_view, monitor_num, is_primary)
{
  is_primary ? ShowPrimaryView() : ShowSecondaryView();
  EnableInputWindow(true);
}

void KylinShield::ShowPrimaryView()
{
  if (primary_layout_)
  {
    if (prompt_view_)
    {
      prompt_view_->scale = scale();
      prompt_layout_->AddView(prompt_view_.GetPointer());
    }

    GrabScreen(false);
    SetLayout(primary_layout_.GetPointer());
    return;
  }

  GrabScreen(true);
  nux::Layout* main_layout = new nux::VLayout();
  primary_layout_ = main_layout;
  SetLayout(primary_layout_.GetPointer());

  prompt_layout_ = new nux::HLayout();

  if (prompt_view_)
  {
    prompt_view_->scale = scale();
    prompt_layout_->AddView(prompt_view_.GetPointer());
  }

  // 10 is just a random number to center the prompt view.
  main_layout->AddSpace(0, 10);
  main_layout->AddLayout(prompt_layout_.GetPointer(), 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FIX);
  main_layout->AddSpace(0, 10);
}

}
}
