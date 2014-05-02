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

#include "LockScreenShield.h"

#include <Nux/VLayout.h>
#include <Nux/HLayout.h>
#include <Nux/PaintLayer.h>

#include "BackgroundSettings.h"
#include "CofView.h"
#include "LockScreenPanel.h"
#include "LockScreenSettings.h"
#include "UserPromptView.h"
#include "unity-shared/UScreen.h"
#include "unity-shared/WindowManager.h"

namespace unity
{
namespace lockscreen
{

Shield::Shield(session::Manager::Ptr const& session_manager, indicator::Indicators::Ptr const& indicators, int monitor_num, bool is_primary)
  : AbstractShield(session_manager, indicators, monitor_num, is_primary)
  , bg_settings_(std::make_shared<BackgroundSettings>())
  , prompt_view_(nullptr)
  , panel_view_(nullptr)
{
  is_primary ? ShowPrimaryView() : ShowSecondaryView();

  EnableInputWindow(true);

  geometry_changed.connect([this] (nux::Area*, nux::Geometry&) { UpdateBackgroundTexture();});

  monitor.changed.connect([this] (int monitor) {
    if (panel_view_)
      panel_view_->monitor = monitor;

    UpdateBackgroundTexture();
  });

  primary.changed.connect([this] (bool is_primary) {
    if (!is_primary)
    {
      UnGrabPointer();
      UnGrabKeyboard();
    }

    is_primary ? ShowPrimaryView() : ShowSecondaryView();
    if (panel_view_) panel_view_->SetInputEventSensitivity(is_primary);
    QueueRelayout();
    QueueDraw();
  });

  mouse_move.connect([this] (int x, int y, int, int, unsigned long, unsigned long) {
    auto const& abs_geo = GetAbsoluteGeometry();
    grab_motion.emit(abs_geo.x + x, abs_geo.y + y);
  });
}

void Shield::UpdateBackgroundTexture()
{
  auto const& monitor_geo = UScreen::GetDefault()->GetMonitorGeometry(monitor);

  if (!background_layer_ || monitor_geo != background_layer_->GetGeometry())
  {
    auto background_texture = bg_settings_->GetBackgroundTexture(monitor);
    background_layer_.reset(new nux::TextureLayer(background_texture->GetDeviceTexture(), nux::TexCoordXForm(), nux::color::White, true));
    SetBackgroundLayer(background_layer_.get());
  }
}

void Shield::CheckCapsLockPrompt()
{
  if (prompt_view_)
    prompt_view_->CheckIfCapsLockOn();
}

void Shield::ShowPrimaryView()
{
  GrabPointer();
  GrabKeyboard();

  if (primary_layout_)
  {
    SetLayout(primary_layout_.GetPointer());
    return;
  }

  nux::Layout* main_layout = new nux::VLayout();
  primary_layout_ = main_layout;
  SetLayout(primary_layout_.GetPointer());

  main_layout->AddView(CreatePanel());

  nux::HLayout* prompt_layout = new nux::HLayout();
  prompt_layout->SetLeftAndRightPadding(2 * Settings::GRID_SIZE);

  prompt_view_ = CreatePromptView();
  prompt_layout->AddView(prompt_view_);

  // 10 is just a random number to center the prompt view.
  main_layout->AddSpace(0, 10);
  main_layout->AddLayout(prompt_layout);
  main_layout->AddSpace(0, 10);
}

void Shield::ShowSecondaryView()
{
  if (cof_layout_)
  {
    SetLayout(cof_layout_.GetPointer());
    return;
  }

  nux::Layout* main_layout = new nux::VLayout();
  cof_layout_ = main_layout;
  SetLayout(cof_layout_.GetPointer());

  // The circle of friends
  CofView* cof_view = new CofView();
  main_layout->AddView(cof_view);
}

Panel* Shield::CreatePanel()
{
  if (!indicators_ || !session_manager_)
    return nullptr;

  panel_view_ = new Panel(monitor, indicators_, session_manager_);
  panel_active_conn_ = panel_view_->active.changed.connect([this] (bool active) {
    if (primary())
    {
      if (active)
      {
        regrab_conn_->disconnect();
        UnGrabPointer();
        UnGrabKeyboard();
      }
      else
      {
        auto& wc = nux::GetWindowCompositor();

        if (!wc.GrabPointerAdd(this) || !wc.GrabKeyboardAdd(this))
        {
          regrab_conn_ = WindowManager::Default().screen_ungrabbed.connect([this] {
            if (primary())
            {
              GrabPointer();
              GrabKeyboard();
            }
            regrab_conn_->disconnect();
          });
        }
      }
    }
  });

  return panel_view_;
}

UserPromptView* Shield::CreatePromptView()
{
  auto* prompt_view = new UserPromptView(session_manager_);

  auto width = 8 * Settings::GRID_SIZE;
  auto height = 3 * Settings::GRID_SIZE;

  prompt_view->SetMinimumWidth(width);
  prompt_view->SetMaximumWidth(width);
  prompt_view->SetMinimumHeight(height);

  return prompt_view;
}

nux::Area* Shield::FindKeyFocusArea(unsigned etype, unsigned long key_sym, unsigned long modifiers)
{
  if (primary)
  {
    grab_key.emit(modifiers, key_sym);

    if (panel_view_ && panel_view_->WillHandleKeyEvent(etype, key_sym, modifiers))
      return panel_view_;

    if (prompt_view_)
    {
      auto* focus_view = prompt_view_->focus_view();

      if (focus_view && focus_view->GetInputEventSensitivity())
        return focus_view;
    }
  }

  return nullptr;
}

bool Shield::AcceptKeyNavFocus()
{
  return false;
}

nux::Area* Shield::FindAreaUnderMouse(nux::Point const& mouse, nux::NuxEventType event_type)
{
  nux::Area* area = BaseWindow::FindAreaUnderMouse(mouse, event_type);

  if (!area && primary)
    return this;

  return area;
}

bool Shield::IsIndicatorOpen() const
{
  return panel_view_ ? panel_view_->active() : false;
}

}
}
