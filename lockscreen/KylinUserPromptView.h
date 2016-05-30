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
* Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
*              handsome_feng <jianfengli@ubuntukylin.com>
*/

#ifndef UNITY_KYLIN_USER_PROMPT_BOX
#define UNITY_KYLIN_USER_PROMPT_BOX

#include "LockScreenAbstractPromptView.h"

namespace nux
{
class VLayout;
class HLayout;
}

namespace unity
{

class StaticCairoText;
class TextInput;
class IconTexture;
class RawPixel;

namespace lockscreen
{

class KylinUserPromptView : public AbstractUserPromptView
{
public:
  KylinUserPromptView(session::Manager::Ptr const& session_manager);

  nux::View* focus_view();

  void AddAvatar(std::string const& avatar_icon, int avatar_size);
  void AddPrompt(std::string const& message, bool visible, PromiseAuthCodePtr const&);
  void AddMessage(std::string const& message, nux::Color const& color);
  void AuthenticationCb(bool authenticated);

protected:
  void Draw(nux::GraphicsEngine& graphics_engine, bool force_draw);
  void DrawContent(nux::GraphicsEngine& graphics_engine, bool force_draw);
  void ResetLayout();
  void UpdateSize();
  bool InspectKeyEvent(unsigned int eventType, unsigned int key_sym, const char* character);
  nux::ObjectPtr<nux::BaseTexture> LoadUserIcon(std::string const& icon_file, int icon_size);

private:
  session::Manager::Ptr session_manager_;
  UserAuthenticatorPam user_authenticator_;
  StaticCairoText* username_;
  nux::VLayout* msg_layout_;
  nux::VLayout* prompt_layout_;
  nux::VLayout* avatar_layout_;
  std::deque<TextInput*> focus_queue_;
  IconTexture* switch_icon_;
  IconTexture* avatar_;
  nux::Property<std::string> avatar_icon_file;

  nux::Geometry cached_focused_geo_;
};

}
}

#endif
