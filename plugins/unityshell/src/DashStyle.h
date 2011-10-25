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

#include <cairo.h>

namespace unity
{

class DashStyle
{
public:

  enum class StockIcon {
    CHECKMARK,
    CROSS,
    GRID_VIEW,
    FLOW_VIEW,
    STAR
  };

  enum class Orientation {
    UP,
    DOWN,
    LEFT,
    RIGHT
  };

  enum class BlendMode {
    NORMAL,
    MULTIPLY,
    SCREEN
  };

  enum class FontWeight {
    LIGHT,
    REGULAR,
    BOLD
  };

  enum class Segment {
    LEFT,
    MIDDLE,
    RIGHT
  };

  enum class Arrow {
    LEFT,
    RIGHT,
    BOTH,
    NONE
  };

  DashStyle ();
  ~DashStyle ();

  static DashStyle& Instance();

  virtual bool Button(cairo_t* cr, nux::ButtonVisualState state,
                      std::string const& label);

  virtual bool StarEmpty(cairo_t* cr, nux::ButtonVisualState state);

  virtual bool StarHalf(cairo_t* cr, nux::ButtonVisualState state);

  virtual bool StarFull(cairo_t* cr, nux::ButtonVisualState state);

  virtual bool MultiRangeSegment(cairo_t*    cr,
                                 nux::ButtonVisualState  state,
                                 std::string const& label,
                                 Arrow       arrow,
                                 Segment     segment);

  virtual bool TrackViewNumber(cairo_t*    cr,
                               nux::ButtonVisualState  state,
                               std::string const& trackNumber);

  virtual bool TrackViewPlay(cairo_t*   cr,
                             nux::ButtonVisualState state);

  virtual bool TrackViewPause(cairo_t*   cr,
                              nux::ButtonVisualState state);

  virtual bool TrackViewProgress(cairo_t* cr);

  virtual bool SeparatorVert(cairo_t* cr);

  virtual bool SeparatorHoriz(cairo_t* cr);

  virtual int GetButtonGarnishSize();

  virtual int GetSeparatorGarnishSize();

  virtual int GetScrollbarGarnishSize();

  void Blur(cairo_t* cr, int size);

  void RoundedRect(cairo_t* cr,
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
