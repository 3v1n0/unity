/*
 * Copyright (C) 2011 Canonical Ltd
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
 * Authored by: Mirco Müller <mirco.mueller@canonical.com
 */

#ifndef DASH_STYLE_H
#define DASH_STYLE_H

#include <cairo.h>

namespace unity
{
  class DashStyle
  {
    public:
      DashStyle ();
      ~DashStyle ();

    virtual bool Scrollbar (cairo_t* cr, nux::View::State state);
    virtual bool TrackEntry (cairo_t* cr, nux::View::State state);
    virtual bool Button (cairo_t* cr, nux::View::State state);
    virtual bool RangeButton (cairo_t* cr, nux::View::State state);
    virtual bool NavBar (cairo_t* cr, nux::View::State state);
    virtual bool Separator (cairo_t* cr, Orientation orientation);
    virtual bool ButtonBackground (cairo_t* cr, nux::View::State state);

    void SetFGColor (float r, float g, float b, float a);
    void SetBGColor (float r, float g, float b, float a);
    void RoundedRect (cairo_t* cr);
    void Blur (cairo_t* cr);
    void Text (cairo_t* cr);
    void Triangle (cairo_t* cr, Triangle::Orientation orientation, nux::View::State state);
    void Star (cairo_t* cr, nux::View::State state);
  };
}

#endif // DASH_STYLE_H
