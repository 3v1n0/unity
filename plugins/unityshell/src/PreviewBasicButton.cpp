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
#include "PreviewBasicButton.h"

namespace unity {

  PreviewBasicButton::PreviewBasicButton (nux::TextureArea *image, NUX_FILE_LINE_DECL)
      : nux::Button (image, NUX_FILE_LINE_PARAM) {
    InitTheme();
  }

  PreviewBasicButton::PreviewBasicButton (const std::string label, NUX_FILE_LINE_DECL)
      : nux::Button (label, NUX_FILE_LINE_PARAM) {
    InitTheme();
  }

  PreviewBasicButton::PreviewBasicButton (const std::string label, nux::TextureArea *image, NUX_FILE_LINE_DECL)
      : nux::Button (label,  image, NUX_FILE_LINE_PARAM) {
    InitTheme();
  }

  PreviewBasicButton::PreviewBasicButton (NUX_FILE_LINE_DECL)
      : nux::Button (NUX_FILE_LINE_PARAM) {
    InitTheme();
  }

  PreviewBasicButton::~PreviewBasicButton() {

  }

  void PreviewBasicButton::InitTheme()
  {
    //FIXME - build theme here - store images, cache them, fun fun fun
  }


  long int PreviewBasicButton::ProcessEvent(nux::IEvent& ievent, long int TraverseInfo, long int ProcessEventInfo) {
    return nux::Button::ProcessEvent(ievent, TraverseInfo, ProcessEventInfo);
  }

  void PreviewBasicButton::Draw(nux::GraphicsEngine& GfxContext, bool force_draw) {
    //FIXME - render theme here
    //FIXME - disable this next line to stop nux drawing the theme
    nux::Button::Draw(GfxContext, force_draw);
  }

  void PreviewBasicButton::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw) {
    //FIXME - i swear i didn't change anything, but suddenly nux stopped drawing the contents of the button
    nux::Button::DrawContent(GfxContext, force_draw);
  }

  void PreviewBasicButton::PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw) {
    nux::Button::PostDraw(GfxContext, force_draw);
  }

}
