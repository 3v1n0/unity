// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2015 Canonical Ltd
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
* Authored by: handsome_feng <jianfengli@ubuntukylin.com>
*/

#include "KylinLockScreenShield.h"

#include <Nux/VLayout.h>
#include <Nux/HLayout.h>
#include <Nux/PaintLayer.h>

#include "CofView.h"
#include "LockScreenSettings.h"
#include "LockScreenAbstractPromptView.h"
#include "unity-shared/UnitySettings.h"

namespace unity
{
namespace lockscreen
{

KylinShield::KylinShield(session::Manager::Ptr const& session_manager,
               Accelerators::Ptr const& accelerators,
               nux::ObjectPtr<AbstractUserPromptView> const& prompt_view,
               int monitor_num, bool is_primary)
  : AbstractShield(session_manager, nullptr, accelerators, prompt_view, monitor_num, is_primary)
  , cof_view_(nullptr)
{
  UpdateScale();
  is_primary ? ShowPrimaryView() : ShowSecondaryView();

  EnableInputWindow(true);

  unity::Settings::Instance().dpi_changed.connect(sigc::mem_fun(this, &KylinShield::UpdateScale));
  geometry_changed.connect([this] (nux::Area*, nux::Geometry&) { UpdateBackgroundTexture();});

  monitor.changed.connect([this] (int) {
    UpdateScale();
    UpdateBackgroundTexture();
  });

  primary.changed.connect([this] (bool is_primary) {
    regrab_conn_->disconnect();
    is_primary ? ShowPrimaryView() : ShowSecondaryView();
    QueueRelayout();
    QueueDraw();
  });

  scale.changed.connect([this] (double scale) {
    if (prompt_view_ && primary())
      prompt_view_->scale = scale;

    if (cof_view_)
      cof_view_->scale = scale;

    if (prompt_layout_)
      prompt_layout_->SetLeftAndRightPadding(2 * Settings::GRID_SIZE.CP(scale));

    background_layer_.reset();
    UpdateBackgroundTexture();
  });

  mouse_move.connect([this] (int x, int y, int, int, unsigned long, unsigned long) {
    auto const& abs_geo = GetAbsoluteGeometry();
    grab_motion.emit(abs_geo.x + x, abs_geo.y + y);
  });
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

void KylinShield::ShowSecondaryView()
{
  if (prompt_layout_)
    prompt_layout_->RemoveChildObject(prompt_view_.GetPointer());

  if (cof_layout_)
  {
    SetLayout(cof_layout_.GetPointer());
    return;
  }

  nux::Layout* main_layout = new nux::VLayout();
  cof_layout_ = main_layout;
  SetLayout(cof_layout_.GetPointer());

  // The circle of friends
  cof_view_ = new CofView();
  cof_view_->scale = scale();
  main_layout->AddView(cof_view_);
}

nux::Area* KylinShield::FindKeyFocusArea(unsigned etype, unsigned long keysym, unsigned long modifiers)
{
  if (primary)
  {
    grab_key.emit(modifiers, keysym);

    if (prompt_view_)
    {
      auto* focus_view = prompt_view_->focus_view();

      if (focus_view && focus_view->GetInputEventSensitivity())
        return focus_view;
    }
  }

  return nullptr;
}

}
}
