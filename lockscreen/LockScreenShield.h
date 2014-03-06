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

#include "LockScreenAbstractShield.h"

namespace unity
{
namespace panel
{
class PanelView;
}
namespace lockscreen
{

class BackgroundSettings;
class UserAuthenticator;
class UserPromptView;

class Shield : public AbstractShield
{
public:
  Shield(session::Manager::Ptr const& session_manager, int monitor, bool is_primary);

  bool AcceptKeyNavFocus() override;
  nux::Area* FindKeyFocusArea(unsigned int, unsigned long, unsigned long) override;
  nux::Area* FindAreaUnderMouse(nux::Point const&, nux::NuxEventType) override;

private:
  void UpdateBackgroundTexture();
  void ShowPrimaryView();
  void ShowSecondaryView();
  nux::View* CreatePanel();
  UserPromptView* CreatePromptView();

  void OnIndicatorEntryShowMenu(std::string const&, unsigned, int, int, unsigned);
  void OnIndicatorEntryActivated(std::string const& panel, std::string const& entry, nux::Geometry const& geo);

  std::shared_ptr<BackgroundSettings> bg_settings_;
  std::unique_ptr<nux::AbstractPaintLayer> background_layer_;
  nux::ObjectPtr<nux::Layout> primary_layout_;
  nux::ObjectPtr<nux::Layout> cof_layout_;
  UserPromptView* prompt_view_;
  panel::PanelView* panel_view_;
};

}
}

#endif