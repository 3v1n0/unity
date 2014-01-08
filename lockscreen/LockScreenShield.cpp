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

#include "config.h"

#include "LockScreenShield.h"

#include "BackgroundSettingsGnome.h" // FIXME: remove this
#include "CofView.h"
#include "unity-shared/PanelStyle.h"
#include "panel/PanelView.h"

#include <Nux/VLayout.h>
#include <Nux/HLayout.h>

#include <Nux/PaintLayer.h>
#include <Nux/Nux.h> // FIXME: remove this

namespace unity 
{
namespace lockscreen
{

Shield::Shield(bool is_primary)
  : primary(is_primary)
  , bg_settings_(new BackgroundSettingsGnome) // FIXME (andy) inject it!
{
  SetLayout(new nux::VLayout());

  UpdateBackgroundTexture();
  primary ? ShowPrimaryView() : ShowSecondaryView();

  mouse_enter.connect(sigc::mem_fun(this, &Shield::OnMouseEnter));
  mouse_leave.connect(sigc::mem_fun(this, &Shield::OnMouseLeave));
  primary.changed.connect(sigc::mem_fun(this, &Shield::OnPrimaryChanged));

  // Move in a method
  bg_settings_->bg_changed.connect([this](){
    UpdateBackgroundTexture();
    QueueDraw();
  });
}

void Shield::UpdateBackgroundTexture()
{
  auto background_texture = bg_settings_->GetBackgroundTexture(nux::Size(GetBaseWidth(), GetBaseHeight()), true);
  background_layer_.reset(new nux::TextureLayer(background_texture->GetDeviceTexture(), nux::TexCoordXForm(), nux::color::White, true));
  SetBackgroundLayer(background_layer_.get());
}

void Shield::OnMouseEnter(int /*x*/, int /*y*/, unsigned long /**/, unsigned long /**/)
{
  // FIXME: does not work well!
  //primary = true;
}

void Shield::OnMouseLeave(int /*x*/, int /**/, unsigned long /**/, unsigned long /**/)
{
  //primary = false;
}

void Shield::OnPrimaryChanged(bool value)
{
  if (primary)
    ShowPrimaryView();
  else
    ShowSecondaryView();

  QueueDraw();
  QueueRelayout();
}

void Shield::ShowPrimaryView()
{
  nux::Layout* main_layout = GetLayout();
  main_layout->Clear();

  PanelView* view = new PanelView(this, std::make_shared<indicator::DBusIndicators>(true));
  view->SetMaximumHeight(panel::Style::Instance().panel_height);
  view->SetOpacity(0.5);

  main_layout->AddView(view);

}

void Shield::ShowSecondaryView()
{
  nux::Layout* main_layout = GetLayout();
  main_layout->Clear();

  CofView* cof_view = new CofView();
  main_layout->AddView(cof_view);
}

}
}
