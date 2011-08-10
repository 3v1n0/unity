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



#ifndef FILTERMULTIRANGEBUTTON_H
#define FILTERMULTIRANGEBUTTON_H

#include <Nux/Nux.h>
#include <Nux/View.h>
#include "Nux/ToggleButton.h"
#include "FilterWidget.h"

namespace unity {

  class FilterMultiRangeButton : public nux::ToggleButton {
  public:
    FilterMultiRangeButton (const std::string label, NUX_FILE_LINE_PROTO);
    FilterMultiRangeButton (NUX_FILE_LINE_PROTO);

    void SetFilter (dash::FilterOption::Ptr filter);
    dash::FilterOption::Ptr GetFilter();

  protected:
    virtual long ComputeLayout2();
    virtual long int ProcessEvent(nux::IEvent& ievent, long int TraverseInfo, long int ProcessEventInfo);
    virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
    virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);
    virtual void PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw);

  private:
    dash::FilterOption::Ptr filter_;

    void InitTheme ();
    void RedrawTheme (nux::Geometry const& geom, cairo_t *cr, nux::State faked_state);

    nux::CairoWrapper *prelight_left_;
    nux::CairoWrapper *active_left_;
    nux::CairoWrapper *normal_left_;

    nux::CairoWrapper *prelight_;
    nux::CairoWrapper *active_;
    nux::CairoWrapper *normal_;

    nux::CairoWrapper *prelight_right_;
    nux::CairoWrapper *active_right_;
    nux::CairoWrapper *normal_right_;
    nux::Geometry cached_geometry_;

  };

}
#endif // FILTERMULTIRANGEBUTTON_H
