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

#include <Nux/View.h>

namespace unity
{

class TextInput;

namespace lockscreen
{

class UserPromptView : public nux::View
{
public:
  UserPromptView(std::string const& name);
  ~UserPromptView() {};

  nux::TextEntry* text_entry();

protected:
  void Draw(nux::GraphicsEngine& graphics_engine, bool force_draw) override;
  void DrawContent(nux::GraphicsEngine& graphics_engine, bool force_draw) override;

private:
  std::shared_ptr<nux::AbstractPaintLayer> bg_layer_;
  TextInput* text_input_;
};

}
}

#endif
