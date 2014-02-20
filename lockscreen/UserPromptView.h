// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2014 Canonical Ltd
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

#ifndef UNITY_USER_PROMPT_BOX
#define UNITY_USER_PROMPT_BOX

#include <memory>
#include <queue>

#include <Nux/Nux.h>
#include <Nux/View.h>
#include <UnityCore/SessionManager.h>

#include "UserAuthenticatorPam.h"
#include "unity-shared/IMTextEntry.h"
#include "unity-shared/SearchBarSpinner.h"

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

class UserPromptView : public nux::View
{
public:
  UserPromptView(session::Manager::Ptr const& session_manager);
  ~UserPromptView() {};

  nux::View* focus_view();

  void AddPrompt(std::string const& message, bool visible, std::shared_ptr<std::promise<std::string>> const& promise);
  void AddMessage(std::string const& message, nux::Color const& color);
  void AuthenticationCb(bool authenticated);

protected:
  void Draw(nux::GraphicsEngine& graphics_engine, bool force_draw) override;
  void DrawContent(nux::GraphicsEngine& graphics_engine, bool force_draw) override;

private:
  void ResetLayout();

  session::Manager::Ptr session_manager_;
  UserAuthenticatorPam user_authenticator_;
  std::shared_ptr<nux::AbstractPaintLayer> bg_layer_;
  nux::VLayout* msg_layout_;
  nux::VLayout* prompt_layout_;
  StaticCairoText* message_;
  StaticCairoText* error_;
  StaticCairoText* invalid_login_;
  std::queue<IMTextEntry*> focus_queue_;
};

}
}

#endif
