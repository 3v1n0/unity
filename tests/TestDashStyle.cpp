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

#include "DashStyle.h"

namespace unity
{
  class DashStyleO : public DashStyle
  {
    public:
      DashStyleO();
      ~DashStyleO();

      bool ScrollbarVert (cairo_t* cr, nux::State state);
      bool TrackView (cairo_t* cr, nux::State state);
  };

  class DashStyleP : public DashStyle
  {
    public:
      DashStyleP();
      ~DashStyleP();

      bool ScrollbarVert (cairo_t* cr, nux::State state);
      bool TrackView (cairo_t* cr, nux::State state);
  };

  DashStyleO::DashStyleO ()
  {
  }

  DashStyleO::~DashStyleO ()
  {
  }

  bool DashStyleO::ScrollbarVert (cairo_t* cr, nux::State state)
  {
    cairo_set_source_rgba (cr, 0.0, 1.0, 0.0, 1.0);
    cairo_paint (cr);

    return true;
  }

  bool DashStyleO::TrackView (cairo_t* cr, nux::State state)
  {
    cairo_set_source_rgba (cr, 0.0, 1.0, 1.0, 1.0);
    cairo_paint (cr);

    return true;
  }

  DashStyleP::DashStyleP ()
  {
  }

  DashStyleP::~DashStyleP ()
  {
  }

  bool DashStyleP::ScrollbarVert (cairo_t* cr, nux::State state)
  {
    cairo_set_source_rgba (cr, 1.0, 0.0, 0.0, 1.0);
    cairo_paint (cr);

    return true;
  }

  bool DashStyleP::TrackView (cairo_t* cr, nux::State state)
  {
    cairo_set_source_rgba (cr, 1.0, 1.0, 0.0, 1.0);
    cairo_paint (cr);

    return true;
  }
}

void wipe (cairo_t* cr)
{
  // sanity check
  if (cairo_status (cr) != CAIRO_STATUS_SUCCESS)
    return;

  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);
  cairo_scale (cr, 1.0, 1.0);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
}

int main (int    argc,
          char** argv)
{
  // setup
  unity::DashStyleP* pDashStyle = new unity::DashStyleP ();
  cairo_surface_t* surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                                         100,
                                                         30);
  cairo_t* cr = cairo_create (surface);
  wipe (cr);
  cairo_surface_write_to_png (cairo_get_target (cr), "/tmp/empty.png");

  // render some elements from different styles to PNG-images
  pDashStyle->Button (cr, nux::NUX_STATE_NORMAL, "Play");
  cairo_surface_write_to_png (cairo_get_target (cr), "/tmp/button-normal.png");
  wipe (cr);

  pDashStyle->Button (cr, nux::NUX_STATE_ACTIVE, "Pause");
  cairo_surface_write_to_png (cairo_get_target (cr), "/tmp/button-active.png");
  wipe (cr);

  pDashStyle->Button (cr, nux::NUX_STATE_PRELIGHT, "Record");
  cairo_surface_write_to_png (cairo_get_target (cr), "/tmp/button-prelight.png");
  wipe (cr);

  pDashStyle->Button (cr, nux::NUX_STATE_SELECTED, "Rewind");
  cairo_surface_write_to_png (cairo_get_target (cr), "/tmp/button-selected.png");
  wipe (cr);

  pDashStyle->Button (cr, nux::NUX_STATE_INSENSITIVE, "Forward");
  cairo_surface_write_to_png (cairo_get_target (cr), "/tmp/button-insensitive.png");
  wipe (cr);

  pDashStyle->StarEmpty (cr, nux::NUX_STATE_NORMAL);
  cairo_surface_write_to_png (cairo_get_target (cr), "/tmp/star-empty.png");
  wipe (cr);

  pDashStyle->StarHalf (cr, nux::NUX_STATE_NORMAL);
  cairo_surface_write_to_png (cairo_get_target (cr), "/tmp/star-half.png");
  wipe (cr);

  pDashStyle->StarFull (cr, nux::NUX_STATE_NORMAL);
  cairo_surface_write_to_png (cairo_get_target (cr), "/tmp/star-full.png");
  wipe (cr);

  pDashStyle->MultiRangeSegment (cr,
                                 nux::NUX_STATE_NORMAL,
                                 "100KB",
                                 unity::DashStyle::ARROW_LEFT,
                                 unity::DashStyle::SEGMENT_LEFT);
  cairo_surface_write_to_png (cairo_get_target (cr), "/tmp/multirange-left.png");
  wipe (cr);

  pDashStyle->MultiRangeSegment (cr,
                                 nux::NUX_STATE_ACTIVE,
                                 "10GB",
                                 unity::DashStyle::ARROW_LEFT,
                                 unity::DashStyle::SEGMENT_MIDDLE);
  cairo_surface_write_to_png (cairo_get_target (cr), "/tmp/multirange-middle-left.png");
  wipe (cr);

  pDashStyle->MultiRangeSegment (cr,
                                 nux::NUX_STATE_ACTIVE,
                                 "1MB",
                                 unity::DashStyle::ARROW_BOTH,
                                 unity::DashStyle::SEGMENT_MIDDLE);
  cairo_surface_write_to_png (cairo_get_target (cr), "/tmp/multirange-middle-both.png");
  wipe (cr);

  pDashStyle->MultiRangeSegment (cr,
                                 nux::NUX_STATE_ACTIVE,
                                 "1TB",
                                 unity::DashStyle::ARROW_RIGHT,
                                 unity::DashStyle::SEGMENT_MIDDLE);
  cairo_surface_write_to_png (cairo_get_target (cr), "/tmp/multirange-middle-right.png");
  wipe (cr);

  pDashStyle->MultiRangeSegment (cr,
                                 nux::NUX_STATE_ACTIVE,
                                 "100KB",
                                 unity::DashStyle::ARROW_LEFT,
                                 unity::DashStyle::SEGMENT_RIGHT);
  cairo_surface_write_to_png (cairo_get_target (cr), "/tmp/multirange-right.png");
  wipe (cr);

  // clean up
  cairo_surface_destroy (surface);
  cairo_destroy (cr);
  delete pDashStyle;

  return 0;
}
