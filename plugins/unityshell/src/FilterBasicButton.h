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



#ifndef FILTERBASICBUTTON_H
#define FILTERBASICBUTTON_H

#include <Nux/Nux.h>
#include <Nux/CairoWrapper.h>
#include <Nux/ToggleButton.h>

#include "FilterWidget.h"
#include "DashStyle.h"

namespace unity {

  class FilterBasicButton : public nux::ToggleButton {
  public:
    FilterBasicButton (nux::TextureArea *image, NUX_FILE_LINE_PROTO);
    FilterBasicButton (const std::string label, NUX_FILE_LINE_PROTO);
    FilterBasicButton (const std::string label, nux::TextureArea *image, NUX_FILE_LINE_PROTO);
    FilterBasicButton (NUX_FILE_LINE_PROTO);
    virtual ~FilterBasicButton();

  protected:
    virtual long ComputeLayout2();
    virtual long int ProcessEvent(nux::IEvent& ievent, long int TraverseInfo, long int ProcessEventInfo);
    virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
    virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);
    virtual void PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw);

    void InitTheme ();
    void RedrawTheme (nux::Geometry const& geom, cairo_t *cr, nux::State faked_state);

    nux::CairoWrapper *prelight_;
    nux::CairoWrapper *active_;
    nux::CairoWrapper *normal_;
    nux::Geometry cached_geometry_;

  private:
    std::string label_;
  };

}
#endif // FILTERBASICBUTTON_H
