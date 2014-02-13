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

  is_primary ? ShowPrimaryView() : ShowSecondaryView();

  EnableInputWindow(true);

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

void Shield::ShowPrimaryView()
{
  GrabPointer();
  GrabKeyboard();

  nux::Layout* main_layout = GetLayout();

  main_layout->AddView(CreatePanel());

  nux::HLayout* prompt_layout = new nux::HLayout();
  prompt_layout->SetLeftAndRightPadding(2 * Settings::GRID_SIZE);
  prompt_layout->AddView(CreatePromptView());

  // 10 is just a random number to center the prompt view.
  main_layout->AddSpace(0, 10);
  main_layout->AddLayout(prompt_layout);
  main_layout->AddSpace(0, 10);
}

void Shield::ShowSecondaryView()
{
  nux::Layout* main_layout = GetLayout();

  // The circle of friends
  CofView* cof_view = new CofView();
  main_layout->AddView(cof_view);
}

nux::View* Shield::CreatePanel()
{
  auto indicators = std::make_shared<indicator::LockscreenDBusIndicators>();

  // Hackish but ok for the moment. Would be nice to have menus without grab.
  indicators->on_entry_show_menu.connect(sigc::mem_fun(this, &Shield::OnIndicatorEntryShowMenu));
  indicators->on_entry_activated.connect(sigc::mem_fun(this, &Shield::OnIndicatorEntryActivated));

  panel::PanelView* panel_view = new panel::PanelView(this, indicators, true);
  panel_view->SetMaximumHeight(panel::Style::Instance().panel_height);

  return panel_view;
}

nux::View* Shield::CreatePromptView()
{
  auto const& real_name = session_manager_->RealName();
  auto const& name = (real_name.empty() ? session_manager_->UserName() : real_name);
  prompt_view_ = new UserPromptView(name);

  auto width = 8 * Settings::GRID_SIZE;
  auto height = 3 * Settings::GRID_SIZE;

  prompt_view_->SetMinimumWidth(width);
  prompt_view_->SetMaximumWidth(width);
  prompt_view_->SetMinimumHeight(height);
  prompt_view_->SetMaximumHeight(height);

  prompt_view_->text_entry()->activated.connect(sigc::mem_fun(this, &Shield::OnPromptActivated));

  return prompt_view_;
}

void Shield::OnIndicatorEntryShowMenu(std::string const&, unsigned, int, int, unsigned)
{
  UnGrabPointer();
  UnGrabKeyboard();
}

void Shield::OnIndicatorEntryActivated(std::string const& entry, nux::Geometry const& geo)
{
  if (entry.empty() and geo.IsNull()) /* on menu closed */
  {
    GrabPointer();
    GrabKeyboard();
  }
}

void Shield::OnPromptActivated()
{
  prompt_view_->SetSpinnerVisible(true);
  prompt_view_->SetSpinnerState(STATE_SEARCHING);

  user_authenticator_->AuthenticateStart(session_manager_->UserName(),
                                         prompt_view_->text_entry()->GetText(),
                                         sigc::mem_fun(this, &Shield::AuthenticationCb));
}

void Shield::AuthenticationCb(bool authenticated)
{
  prompt_view_->SetSpinnerVisible(false);

  if (authenticated)
    session_manager_->unlock_requested.emit();
  else
    prompt_view_->ShowErrorMessage();
}

nux::Area* Shield::FindKeyFocusArea(unsigned int,
                                    unsigned long,
                                    unsigned long)
{
  if (prompt_view_)
    return prompt_view_->text_entry();
  else
    return nullptr;
}

bool Shield::AcceptKeyNavFocus()
{
  return false;
}

}
}
