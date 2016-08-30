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

#include "LockScreenPanel.h"
#include "LockScreenSettings.h"
#include "LockScreenAbstractPromptView.h"

namespace unity
{
namespace lockscreen
{

Shield::Shield(session::Manager::Ptr const& session_manager,
               menu::Manager::Ptr const& menu_manager,
               Accelerators::Ptr const& accelerators,
               nux::ObjectPtr<AbstractUserPromptView> const& prompt_view,
               int monitor_num, bool is_primary)
  : BaseShield(session_manager, accelerators, prompt_view, monitor_num, is_primary)
  , menu_manager_(menu_manager)
  , panel_view_(nullptr)
{
  is_primary ? ShowPrimaryView() : ShowSecondaryView();
  EnableInputWindow(true);

  monitor.changed.connect([this] (int monitor) {
    if (panel_view_)
      panel_view_->monitor = monitor;
  });

  primary.changed.connect([this] (bool is_primary) {
    if (panel_view_) panel_view_->SetInputEventSensitivity(is_primary);
  });
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

Panel* Shield::CreatePanel()
{
  if (!menu_manager_ || !session_manager_)
    return nullptr;

  panel_view_ = new Panel(monitor, menu_manager_, session_manager_);
  panel_view_->active.changed.connect(sigc::track_obj([this] (bool active) {
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
  }, *this));

  return panel_view_;
}

nux::Area* Shield::FindKeyFocusArea(unsigned etype, unsigned long keysym, unsigned long modifiers)
{
  if (primary)
  {
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
  }

  return BaseShield::FindKeyFocusArea(etype, keysym, modifiers);
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
