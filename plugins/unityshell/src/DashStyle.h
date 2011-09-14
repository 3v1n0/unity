// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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

#include <Nux/Nux.h>
#include <Nux/View.h>
#include <Nux/AbstractButton.h>

#include <json-glib/json-glib.h>
#include <cairo.h>

namespace unity
{
  class DashStyle
  {
    public:

      typedef enum {
        STOCK_ICON_CHECKMARK = 0,
        STOCK_ICON_CROSS,
        STOCK_ICON_GRID_VIEW,
        STOCK_ICON_FLOW_VIEW,
        STOCK_ICON_STAR
      } StockIcon;

      typedef enum {
        ORIENTATION_UP = 0,
        ORIENTATION_DOWN,
        ORIENTATION_LEFT,
        ORIENTATION_RIGHT
      } Orientation;

      typedef enum {
        BLEND_MODE_NORMAL = 0,
        BLEND_MODE_MULTIPLY,
        BLEND_MODE_SCREEN
    } BlendMode;

      typedef enum {
        FONT_WEIGHT_LIGHT = 0,
        FONT_WEIGHT_REGULAR,
        FONT_WEIGHT_BOLD
    } FontWeight;

      typedef enum {
        SEGMENT_LEFT = 0,
		SEGMENT_MIDDLE,
		SEGMENT_RIGHT
	  } Segment;

      typedef enum {
        ARROW_LEFT = 0,
		ARROW_RIGHT,
		ARROW_BOTH,
		ARROW_NONE
	  } Arrow;

      DashStyle ();
      ~DashStyle ();

      static DashStyle* GetDefault();

    virtual bool Button (cairo_t* cr, nux::State state, std::string label);

    virtual bool StarEmpty (cairo_t* cr, nux::State state);

    virtual bool StarHalf (cairo_t* cr, nux::State state);

    virtual bool StarFull (cairo_t* cr, nux::State state);

    virtual bool MultiRangeSegment (cairo_t*    cr,
                                    nux::State  state,
                                    std::string label,
                                    Arrow       arrow,
                                    Segment     segment);

    virtual bool TrackViewNumber (cairo_t*    cr,
                                  nux::State  state,
                                  std::string trackNumber);

    virtual bool TrackViewPlay (cairo_t*   cr,
                                nux::State state);

    virtual bool TrackViewPause (cairo_t*   cr,
                                 nux::State state);

    virtual bool TrackViewProgress (cairo_t* cr);

    virtual bool SeparatorVert (cairo_t* cr);

    virtual bool SeparatorHoriz (cairo_t* cr);

    virtual int GetButtonGarnishSize ();

    virtual int GetSeparatorGarnishSize ();

    virtual int GetScrollbarGarnishSize ();

    void Blur (cairo_t* cr, int size);

    void RoundedRect (cairo_t* cr,
                      double   aspect,
                      double   x,
                      double   y,
                      double   cornerRadius,
                      double   width,
                      double   height,
                      bool     align);

  private:
    class Impl;
    Impl* pimpl;
  };
}

#endif // DASH_STYLE_H
