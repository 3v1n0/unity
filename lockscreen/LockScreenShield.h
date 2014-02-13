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

#include <NuxCore/Property.h>
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/GLibSignal.h>
#include <UnityCore/SessionManager.h>

#include "unity-shared/MockableBaseWindow.h"

namespace unity
{
namespace lockscreen
{

class BackgroundSettings;
class UserAuthenticator;
class UserPromptView;

class Shield : public MockableBaseWindow
{
public:
  Shield(session::Manager::Ptr const& session_manager, bool is_primary);

  nux::Property<bool> primary;

  nux::Area* FindKeyFocusArea(unsigned int key_symbol,
                                   unsigned long x11_key_code,
                                   unsigned long special_keys_state) override;

  bool AcceptKeyNavFocus() override {return false;}


private:
  void UpdateBackgroundTexture();
  void ShowPrimaryView();
  void ShowSecondaryView();

  void OnIndicatorEntryShowMenu(std::string const&, unsigned, int, int, unsigned);
  void OnIndicatorEntryActivated(std::string const& entry, nux::Geometry const& geo);

  session::Manager::Ptr session_manager_;
  std::shared_ptr<BackgroundSettings> bg_settings_;
  std::unique_ptr<nux::AbstractPaintLayer> background_layer_;
  std::shared_ptr<UserAuthenticator> user_authenticator_;

  UserPromptView* prompt_view_;
};

}
}

#endif