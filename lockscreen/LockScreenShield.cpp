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
#include "unity-shared/UnitySettings.h"
#include "unity-shared/WindowManager.h"

namespace unity
{
namespace lockscreen
{
namespace
{
const unsigned MAX_GRAB_WAIT = 100;
}

Shield::Shield(session::Manager::Ptr const& session_manager,
               indicator::Indicators::Ptr const& indicators,
               Accelerators::Ptr const& accelerators,
               nux::ObjectPtr<UserPromptView> const& prompt_view,
               int monitor_num, bool is_primary)
  : AbstractShield(session_manager, indicators, accelerators, prompt_view, monitor_num, is_primary)
  , bg_settings_(std::make_shared<BackgroundSettings>())
  , panel_view_(nullptr)
  , cof_view_(nullptr)
{
  UpdateScale();
  is_primary ? ShowPrimaryView() : ShowSecondaryView();

  EnableInputWindow(true);

  unity::Settings::Instance().dpi_changed.connect(sigc::mem_fun(this, &Shield::UpdateScale));
  geometry_changed.connect([this] (nux::Area*, nux::Geometry&) { UpdateBackgroundTexture();});

  monitor.changed.connect([this] (int monitor) {
    UpdateScale();

    if (panel_view_)
      panel_view_->monitor = monitor;

    UpdateBackgroundTexture();
  });

  primary.changed.connect([this] (bool is_primary) {
    regrab_conn_->disconnect();
    is_primary ? ShowPrimaryView() : ShowSecondaryView();
    if (panel_view_) panel_view_->SetInputEventSensitivity(is_primary);
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

void Shield::UpdateScale()
{
  scale = unity::Settings::Instance().em(monitor)->DPIScale();
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

void Shield::GrabScreen(bool cancel_on_failure)
{
  auto& wc = nux::GetWindowCompositor();

  if (wc.GrabPointerAdd(this) && wc.GrabKeyboardAdd(this))
  {
    regrab_conn_->disconnect();
    regrab_timeout_.reset();
    grabbed.emit();
  }
  else
  {
    auto const& retry_cb = sigc::bind(sigc::mem_fun(this, &Shield::GrabScreen), false);
    regrab_conn_ = WindowManager::Default().screen_ungrabbed.connect(retry_cb);

    if (cancel_on_failure)
    {
      regrab_timeout_.reset(new glib::Timeout(MAX_GRAB_WAIT, [this] {
        grab_failed.emit();
        return false;
      }));
    }
  }
}

bool Shield::HasGrab() const
{
  auto& wc = nux::GetWindowCompositor();
  return (wc.GetPointerGrabArea() == this && wc.GetKeyboardGrabArea() == this);
}

void Shield::ShowPrimaryView()
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

  main_layout->AddView(CreatePanel());

  prompt_layout_ = new nux::HLayout();
  prompt_layout_->SetLeftAndRightPadding(2 * Settings::GRID_SIZE.CP(scale));

  if (prompt_view_)
  {
    prompt_view_->scale = scale();
    prompt_layout_->AddView(prompt_view_.GetPointer());
  }

  // 10 is just a random number to center the prompt view.
  main_layout->AddSpace(0, 10);
  main_layout->AddLayout(prompt_layout_.GetPointer());
  main_layout->AddSpace(0, 10);
}

void Shield::ShowSecondaryView()
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
        GrabScreen(false);
      }
    }
  });

  return panel_view_;
}

nux::Area* Shield::FindKeyFocusArea(unsigned etype, unsigned long keysym, unsigned long modifiers)
{
  if (primary)
  {
    grab_key.emit(modifiers, keysym);

    if (accelerators_)
    {
      if (etype == nux::EVENT_KEY_DOWN)
      {
        if (accelerators_->HandleKeyPress(keysym, modifiers))
          return panel_view_;
      }
      else if (etype == nux::EVENT_KEY_UP)
      {
        if (accelerators_->HandleKeyRelease(keysym, modifiers))
          return panel_view_;
      }
    }

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

void Shield::ActivatePanel()
{
  if (panel_view_)
    panel_view_->ActivatePanel();
}

}
}
