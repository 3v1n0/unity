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

#include "BackgroundSettings.h" // FIXME: remove this
#include "CofView.h"
#include "LockScreenSettings.h"
#include "UserAuthenticatorPam.h"
#include "UserPromptView.h"
#include "unity-shared/PanelStyle.h"
#include "unity-shared/TextInput.h"
#include "panel/PanelView.h"

#include <Nux/VLayout.h>
#include <Nux/HLayout.h>

#include <Nux/PaintLayer.h>
#include <Nux/Nux.h> // FIXME: remove this

namespace unity 
{
namespace lockscreen
{

Shield::Shield(session::Manager::Ptr const& session_manager, bool is_primary)
  : primary(is_primary)
  , session_manager_(session_manager)
  , bg_settings_(new BackgroundSettings) // FIXME (andy) inject it!
  , user_authenticator_(new UserAuthenticatorPam)
  , prompt_view_(nullptr)
{
  SetLayout(new nux::VLayout());

  primary ? ShowPrimaryView() : ShowSecondaryView();

  EnableInputWindow(true);

  mouse_enter.connect(sigc::mem_fun(this, &Shield::OnMouseEnter));
  mouse_leave.connect(sigc::mem_fun(this, &Shield::OnMouseLeave));
  primary.changed.connect(sigc::mem_fun(this, &Shield::OnPrimaryChanged));

  geometry_changed.connect([this](nux::Area*, nux::Geometry&) {
    UpdateBackgroundTexture();
  });

}

void Shield::UpdateBackgroundTexture()
{
  auto background_texture = bg_settings_->GetBackgroundTexture(nux::Size(GetBaseWidth(), GetBaseHeight()), true, true);
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
  GrabPointer();
  GrabKeyboard();

  nux::Layout* main_layout = GetLayout();
  main_layout->Clear();

  auto indicators = std::make_shared<indicator::DBusIndicators>(/* lockscreen mode */ true);

  // Hackish but ok for the moment.
  indicators->on_entry_show_menu.connect(sigc::mem_fun(this, &Shield::OnEntryShowMenu));
  indicators->on_entry_activated.connect(sigc::mem_fun(this, &Shield::OnEntryActivated));

  panel::PanelView* panel_view = new panel::PanelView(this, indicators, true);
  panel_view->SetMaximumHeight(panel::Style::Instance().panel_height);
  panel_view->SetOpacity(0.5);

  main_layout->AddView(panel_view);

  nux::HLayout* prompt_layout = new nux::HLayout();
  prompt_layout->SetLeftAndRightPadding(2*Settings::GRID_SIZE); // FIXME (andy)

  auto const& real_name = session_manager_->RealName();
  auto const& name = (real_name.empty() ? session_manager_->UserName() : real_name);
  prompt_view_ = new UserPromptView(name);

  prompt_view_->SetMinimumWidth(8*Settings::GRID_SIZE);
  prompt_view_->SetMaximumWidth(8*Settings::GRID_SIZE);
  prompt_view_->SetMinimumHeight(3*Settings::GRID_SIZE);
  prompt_view_->SetMaximumHeight(3*Settings::GRID_SIZE);
  prompt_layout->AddView(prompt_view_);

  main_layout->AddSpace(0, 10);
  main_layout->AddLayout(prompt_layout);
  main_layout->AddSpace(0, 10);

  prompt_view_->text_entry()->activated.connect([this](){
    user_authenticator_->AuthenticateStart(session_manager_->UserName(), prompt_view_->text_entry()->GetText(), [this](bool authenticated) {
      if (authenticated)
      {
        session_manager_->unlock_requested.emit();
      }
      else
      {
        prompt_view_->ShowErrorMessage();
      }
    });
  });
}

void Shield::OnEntryShowMenu(std::string const&, unsigned, int, int, unsigned)
{
  UnGrabPointer();
  UnGrabKeyboard();
}

void Shield::OnEntryActivated(std::string const& entry, nux::Geometry const& geo)
{
  if (entry.empty() and geo.IsNull())
  {
    GrabPointer();
    GrabKeyboard();
  }
}

void Shield::ShowSecondaryView()
{
  nux::Layout* main_layout = GetLayout();
  main_layout->Clear();

  CofView* cof_view = new CofView();
  main_layout->AddView(cof_view);
}

nux::Area* Shield::FindKeyFocusArea(unsigned int key_symbol,
                                    unsigned long x11_key_code,
                                    unsigned long special_keys_state)
{
  if (prompt_view_)
    return prompt_view_->text_entry();
  else
    return nullptr;
}

}
}
