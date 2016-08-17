// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2015 Canonical Ltd
 *               2015, National University of Defense Technology(NUDT) & Kylin Ltd
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
 * Authored by: handsome_feng <jianfengli@ubuntukylin.com>
 */

#ifndef UNITY_LOCKSCREEN_ABSTRACT_USER_PROMPT_H
#define UNITY_LOCKSCREEN_ABSTRACT_USER_PROMPT_H

#include <memory>
#include <deque>

#include <Nux/Nux.h>
#include <Nux/View.h>
#include <Nux/VLayout.h>
#include "UnityCore/SessionManager.h"

#include "UserAuthenticatorPam.h"
#include "unity-shared/IMTextEntry.h"

namespace nux
{
class VLayout;
}
namespace unity
{

class StaticCairoText;
class TextInput;

namespace lockscreen
{

class AbstractUserPromptView : public nux::View
{
public:
  AbstractUserPromptView(session::Manager::Ptr const& session_manager)
    : nux::View(NUX_TRACKER_LOCATION)
    , scale(1.0)
    , session_manager_(session_manager)
  {}

  nux::Property<double> scale;

  virtual nux::View* focus_view() = 0;

  virtual void AuthenticationCb(bool authenticated) = 0;
  virtual void ResetLayout() = 0;
  virtual void UpdateSize() = 0;

protected:
  session::Manager::Ptr session_manager_;
  UserAuthenticatorPam user_authenticator_;
  std::shared_ptr<nux::AbstractPaintLayer> bg_layer_;
  StaticCairoText* username_;
  nux::VLayout* msg_layout_;
  nux::VLayout* prompt_layout_;
  std::deque<TextInput*> focus_queue_;

  nux::Geometry cached_focused_geo_;
};

} // lockscreen
} // unity

#endif // UNITY_LOCKSCREEN_ABSTRACT_USER_PROMPT_H
