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

#include "unity-shared/MockableBaseWindow.h"

namespace unity
{
namespace lockscreen
{

class BackgroundSettings;
class UserPromptView;

class Shield : public MockableBaseWindow
{
public:
  Shield(bool is_primary);
  ~Shield() {};

  nux::Property<bool> primary;

  nux::Area* FindKeyFocusArea(unsigned int key_symbol,
                                   unsigned long x11_key_code,
                                   unsigned long special_keys_state) override;

  bool AcceptKeyNavFocus() override {return false;}


private:
  void UpdateBackgroundTexture();
  void ShowPrimaryView();
  void ShowSecondaryView();

  void OnMouseEnter(int /*x*/, int /*y*/, unsigned long /**/, unsigned long /**/);
  void OnMouseLeave(int /*x*/, int /**/, unsigned long /**/, unsigned long /**/);
  void OnPrimaryChanged(bool value);

  std::shared_ptr<BackgroundSettings> bg_settings_;
  std::unique_ptr<nux::AbstractPaintLayer> background_layer_;

  UserPromptView* prompt_view_;
};

}
}

#endif