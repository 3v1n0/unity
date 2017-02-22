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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#include "LockScreenBaseShield.h"

#include "BackgroundSettings.h"
#include "CofView.h"
#include "LockScreenAbstractPromptView.h"
#include "LockScreenSettings.h"
#include "unity-shared/MockableBaseWindow.h"
#include "unity-shared/UnitySettings.h"
#include "unity-shared/UScreen.h"
#include "unity-shared/WindowManager.h"

namespace unity
{
namespace lockscreen
{
namespace
{
const unsigned MAX_GRAB_WAIT = 100;
}

BaseShield::BaseShield(session::Manager::Ptr const& session,
                       Accelerators::Ptr const& accelerators,
                       nux::ObjectPtr<AbstractUserPromptView> const& prompt_view,
                       int monitor_num, bool is_primary)
  : MockableBaseWindow("Unity Lockscreen")
  , primary(is_primary)
  , monitor(monitor_num)
  , scale(1.0)
  , session_manager_(session)
  , accelerators_(accelerators)
  , prompt_view_(prompt_view)
  , bg_settings_(std::make_shared<BackgroundSettings>())
  , cof_view_(nullptr)
{
  UpdateScale();

  unity::Settings::Instance().dpi_changed.connect(sigc::mem_fun(this, &BaseShield::UpdateScale));
  geometry_changed.connect([this] (nux::Area*, nux::Geometry&) { UpdateBackgroundTexture();});

  monitor.changed.connect([this] (int monitor) {
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
}

bool BaseShield::HasGrab() const
{
  auto& wc = nux::GetWindowCompositor();
  return (wc.GetPointerGrabArea() == this && wc.GetKeyboardGrabArea() == this);
}

nux::Area* BaseShield::FindKeyFocusArea(unsigned etype, unsigned long keysym, unsigned long modifiers)
{
  if (primary && prompt_view_)
  {
    auto* focus_view = prompt_view_->focus_view();

    if (focus_view && focus_view->GetInputEventSensitivity())
      return focus_view;
  }

  return nullptr;
}

nux::Area* BaseShield::FindAreaUnderMouse(nux::Point const& mouse, nux::NuxEventType event_type)
{
  nux::Area* area = BaseWindow::FindAreaUnderMouse(mouse, event_type);

  if (!area && primary)
    return this;

  return area;
}

void BaseShield::GrabScreen(bool cancel_on_failure)
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
    auto const& retry_cb = sigc::bind(sigc::mem_fun(this, &BaseShield::GrabScreen), false);
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

void BaseShield::UpdateBackgroundTexture()
{
  auto const& monitor_geo = UScreen::GetDefault()->GetMonitorGeometry(monitor);

  if (!background_layer_ || monitor_geo != background_layer_->GetGeometry())
  {
    auto background_texture = bg_settings_->GetBackgroundTexture(monitor);
    background_layer_.reset(new nux::TextureLayer(background_texture->GetDeviceTexture(), nux::TexCoordXForm(), nux::color::White, true));
    background_layer_->SetGeometry(monitor_geo);
    SetBackgroundLayer(background_layer_.get());
  }
}

void BaseShield::UpdateScale()
{
  scale = unity::Settings::Instance().em(monitor)->DPIScale();
}

void BaseShield::ShowSecondaryView()
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

} // lockscreen
} // unity
