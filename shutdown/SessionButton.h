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
* Authored by: Marco Trevisan (Treviño) <marco@ubuntu.com>
*/

#ifndef UNITYSHELL_SESSION_BUTTON_H
#define UNITYSHELL_SESSION_BUTTON_H

#include <Nux/Nux.h>
#include <Nux/View.h>
#include <NuxCore/Property.h>

#include "unity-shared/IconTexture.h"
#include "unity-shared/Introspectable.h"
#include "unity-shared/StaticCairoText.h"

namespace unity
{
namespace session
{

class Button : public nux::View, public debug::Introspectable
{
  NUX_DECLARE_OBJECT_TYPE(Button, nux::View);
public:
  enum class Action
  {
    LOCK,
    LOGOUT,
    SUSPEND,
    HIBERNATE,
    SHUTDOWN,
    REBOOT
  };

  Button(Action, NUX_FILE_LINE_PROTO);

  nux::Property<bool> highlighted;
  nux::ROProperty<Action> action;
  nux::ROProperty<std::string> label;

  sigc::signal<void> activated;

protected:
  void Draw(nux::GraphicsEngine&, bool force);

  // Introspectable methods
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

private:
  friend class TestSessionButton;

  Action action_;
  IconTexture* image_view_;
  StaticCairoText* label_view_;
  nux::ObjectPtr<nux::BaseTexture> normal_tex_;
  nux::ObjectPtr<nux::BaseTexture> highlight_tex_;
};

} // namespace session

} // namespace unity

#endif // UNITYSHELL_SESSION_BUTTON_H

