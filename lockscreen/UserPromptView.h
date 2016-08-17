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
#include <deque>

#include <Nux/Nux.h>
#include <Nux/View.h>
#include "UnityCore/SessionManager.h"

#include "LockScreenAbstractPromptView.h"
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

class UserPromptView : public AbstractUserPromptView
{
public:
  UserPromptView(session::Manager::Ptr const& session_manager);

  nux::View* focus_view();

  void AddButton(std::string const& text, std::function<void()> const& cb);
  void AddPrompt(std::string const& message, bool visible, PromiseAuthCodePtr const&);
  void AddMessage(std::string const& message, nux::Color const& color);
  void AuthenticationCb(bool authenticated);

protected:
  void Draw(nux::GraphicsEngine& graphics_engine, bool force_draw) override;
  void DrawContent(nux::GraphicsEngine& graphics_engine, bool force_draw) override;
  bool InspectKeyEvent(unsigned int eventType, unsigned int key_sym, const char* character) override;

private:
  void ResetLayout();
  void UpdateSize();
  void EnsureBGLayer();

  void ShowAuthenticated(bool successful);
  void StartAuthentication();
  void DoUnlock();

  session::Manager::Ptr session_manager_;
  UserAuthenticatorPam user_authenticator_;
  std::shared_ptr<nux::AbstractPaintLayer> bg_layer_;
  StaticCairoText* username_;
  nux::VLayout* msg_layout_;
  nux::VLayout* prompt_layout_;
  nux::VLayout* button_layout_;
  std::deque<TextInput*> focus_queue_;

  nux::Geometry cached_focused_geo_;

  bool prompted_;
  bool unacknowledged_messages_;
};

}
}

#endif
