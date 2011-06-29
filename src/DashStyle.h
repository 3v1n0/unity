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

#include "Nux/Nux.h"

#include <cairo.h>

#define STATES   5
#define CHANNELS 3
#define R        0
#define G        1
#define B        2

namespace nux
{
  enum State
  {
    NUX_STATE_NORMAL,
    NUX_STATE_ACTIVE,
    NUX_STATE_PRELIGHT,
    NUX_STATE_SELECTED,
    NUX_STATE_INSENSITIVE
  };
}

namespace unity
{
  class DashStyle
  {
    public:

      typedef enum {
        STOCK_ICON_CHECKMARK,
        STOCK_ICON_CROSS,
        STOCK_ICON_GRID_VIEW,
        STOCK_ICON_FLOW_VIEW,
        STOCK_ICON_STAR,
      } StockIcon;

      typedef enum {
        ORIENTATION_UP,
        ORIENTATION_DOWN,
        ORIENTATION_LEFT,
        ORIENTATION_RIGHT,
      } Orientation;

      DashStyle ();
      ~DashStyle ();

    virtual bool ButtonWithIcon (cairo_t*             cr,
                                 DashStyle::StockIcon id,
                                 nux::State           state);
    virtual bool ButtonWithLabel (cairo_t*    cr,
                                  const char* text,
                                  nux::State  state);
    virtual bool ButtonWithIconAndLabel (cairo_t*             cr,
                                         DashStyle::StockIcon iconId,
                                         const char*          text,
                                         nux::State           state);

    virtual bool ButtonBackground (cairo_t* cr, nux::State state);

    virtual bool MultiRangeBar (cairo_t* cr, nux::State state);

    virtual bool RatingsBar (cairo_t* cr, nux::State state);

    virtual bool LensNavBar (cairo_t* cr, nux::State state);

    virtual bool ScrollbarVert (cairo_t* cr, nux::State state);
    virtual bool ScrollbarHoriz (cairo_t* cr, nux::State state);

    virtual bool SeparatorVert (cairo_t* cr);
    virtual bool SeparatorHoriz (cairo_t* cr);

    virtual bool TrackView (cairo_t* cr, nux::State state);

    virtual bool PreviewHeading (cairo_t* cr);

    void RoundedRect (cairo_t* cr,
                      double   aspect,
                      double   x,
                      double   y,
                      double   cornerRadius,
                      double   width,
                      double   height,
                      bool     align);

	void DrawIcon (cairo_t*             cr,
	               DashStyle::StockIcon id,
	               double               opacity,
	               int                  blurSize);

    void Blur (cairo_t* cr, int size);
    void Text (cairo_t* cr);
    void Triangle (cairo_t*               cr,
                   DashStyle::Orientation orientation,
                   nux::State             state);
    void Star (cairo_t* cr, nux::State state);

    private:
      double _buttonIconOpacity[STATES];
      int    _buttonIconBlurSize[STATES];
  };
}

#endif // DASH_STYLE_H
