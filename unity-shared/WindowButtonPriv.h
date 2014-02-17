// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010-2012 Canonical Ltd
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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 *              Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 */

#ifndef WINDOW_BUTTON_PRIVATE_H
#define WINDOW_BUTTON_PRIVATE_H

#include <Nux/Button.h>

#include "unity-shared/DecorationStyle.h"
#include "unity-shared/Introspectable.h"
#include "unity-shared/PanelStyle.h"
#include "unity-shared/RawPixel.h"
#include "unity-shared/UBusWrapper.h"
#include "unity-shared/UnitySettings.h"

namespace unity
{
class WindowButtons;

namespace internal
{

class WindowButton : public nux::Button, public debug::Introspectable
{
  // A single window button
public:
  WindowButton(panel::WindowButtonType type);

  panel::WindowButtonType GetType() const;
  void SetVisualState(nux::ButtonVisualState new_state);

  void OnMonitorChanged(int monitor);

  nux::RWProperty<bool> enabled;
  nux::Property<bool> overlay_mode;

protected:
  void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  std::string GetName() const;
  void AddProperties(debug::IntrospectionData&);

private:
  void UpdateSize();
  void LoadImages();
  bool EnabledSetter(bool enabled);
  static nux::ObjectPtr<nux::BaseTexture> GetDashWindowButton(panel::WindowButtonType type, panel::WindowState state);

  void UpdateGeometry();

  inline WindowButtons* Parent() const
  {
    return static_cast<WindowButtons*>(GetParentObject());
  }

private:
  panel::WindowButtonType type_;

  int monitor_;
  EMConverter cv_;

  nux::ObjectPtr<nux::BaseTexture> normal_tex_;
  nux::ObjectPtr<nux::BaseTexture> prelight_tex_;
  nux::ObjectPtr<nux::BaseTexture> pressed_tex_;
  nux::ObjectPtr<nux::BaseTexture> unfocused_tex_;
  nux::ObjectPtr<nux::BaseTexture> unfocused_prelight_tex_;
  nux::ObjectPtr<nux::BaseTexture> unfocused_pressed_tex_;
  nux::ObjectPtr<nux::BaseTexture> disabled_tex_;
  nux::ObjectPtr<nux::BaseTexture> normal_dash_tex_;
  nux::ObjectPtr<nux::BaseTexture> prelight_dash_tex_;
  nux::ObjectPtr<nux::BaseTexture> pressed_dash_tex_;
  nux::ObjectPtr<nux::BaseTexture> disabled_dash_tex_;
};

} // internal namespace
} // unity namespace

#endif // WINDOW_BUTTON_PRIVATE_H
