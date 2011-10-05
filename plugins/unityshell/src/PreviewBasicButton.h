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



#ifndef PREVIEWBASICBUTTON_H
#define PREVIEWBASICBUTTON_H

#include <Nux/Nux.h>
#include <Nux/Button.h>
#include <Nux/CairoWrapper.h>
#include "FilterWidget.h"

namespace unity {

  class PreviewBasicButton : public nux::Button {
  public:
    PreviewBasicButton (nux::TextureArea *image, NUX_FILE_LINE_PROTO);
    PreviewBasicButton (const std::string label, NUX_FILE_LINE_PROTO);
    PreviewBasicButton (const std::string label, nux::TextureArea *image, NUX_FILE_LINE_PROTO);
    PreviewBasicButton (NUX_FILE_LINE_PROTO);
    virtual ~PreviewBasicButton();

  protected:
    virtual long ComputeContentSize ();
    virtual long int ProcessEvent(nux::IEvent& ievent, long int TraverseInfo, long int ProcessEventInfo);
    virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
    virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);
    virtual void PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw);

  private:
    void InitTheme ();
    void RedrawTheme (nux::Geometry const& geom, cairo_t *cr, nux::ButtonVisualState faked_state);

    nux::CairoWrapper *prelight_;
    nux::CairoWrapper *active_;
    nux::CairoWrapper *normal_;
    nux::Geometry cached_geometry_;
  };

}
#endif // PREVIEWBASICBUTTON_H
