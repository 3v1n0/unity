/*
 * Copyright 2011 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 *
 */
#include "config.h"
#include "Nux/Nux.h"
#include "FilterBasicButton.h"

namespace unity {

  FilterBasicButton::FilterBasicButton (nux::TextureArea *image, NUX_FILE_LINE_DECL)
      : nux::ToggleButton (image, NUX_FILE_LINE_PARAM) {
    InitTheme();
  }

  FilterBasicButton::FilterBasicButton (const std::string label, NUX_FILE_LINE_DECL)
      : nux::ToggleButton (label, NUX_FILE_LINE_PARAM) {
    InitTheme();
  }

  FilterBasicButton::FilterBasicButton (const std::string label, nux::TextureArea *image, NUX_FILE_LINE_DECL)
      : nux::ToggleButton (label,  image, NUX_FILE_LINE_PARAM) {
    InitTheme();
  }

  FilterBasicButton::FilterBasicButton (NUX_FILE_LINE_DECL)
      : nux::ToggleButton (NUX_FILE_LINE_PARAM) {
    InitTheme();
  }

  FilterBasicButton::~FilterBasicButton() {

  }

  void FilterBasicButton::InitTheme()
  {
    //FIXME - build theme here - store images, cache them, fun fun fun
  }


  long int FilterBasicButton::ProcessEvent(nux::IEvent& ievent, long int TraverseInfo, long int ProcessEventInfo) {
    return nux::Button::ProcessEvent(ievent, TraverseInfo, ProcessEventInfo);
  }

  void FilterBasicButton::Draw(nux::GraphicsEngine& GfxContext, bool force_draw) {
    //FIXME - render theme here
    //FIXME - disable this next line to stop nux drawing the theme
    nux::Button::Draw(GfxContext, force_draw);
  }

  void FilterBasicButton::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw) {
    //FIXME - i swear i didn't change anything, but suddenly nux stopped drawing the contents of the button
    nux::Button::DrawContent(GfxContext, force_draw);
  }

  void FilterBasicButton::PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw) {
    nux::Button::PostDraw(GfxContext, force_draw);
  }

}
