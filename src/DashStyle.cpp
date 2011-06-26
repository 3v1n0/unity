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

#include <math.h>

#include "DashStyle.h"

namespace unity
{
  DashStyle::DashStyle ()
  {
  }

  DashStyle::~DashStyle ()
  {
  }

  bool DashStyle::Scrollbar (cairo_t* cr, nux::View::State state)
  {
    // sanity check
    if (cairo_status (cr) != CAIRO_STATUS_SUCCESS)
      return false;

  }

  bool DashStyle::TrackEntry (cairo_t* cr, nux::View::State state)
  {
    // sanity check
    if (cairo_status (cr) != CAIRO_STATUS_SUCCESS)
      return false;

  }

  bool DashStyle::Button (cairo_t* cr, nux::View::State state)
  {
    // sanity check
    if (cairo_status (cr) != CAIRO_STATUS_SUCCESS)
      return false;

  }

  bool DashStyle::RangeButton (cairo_t* cr, nux::View::State state)
  {
    // sanity check
    if (cairo_status (cr) != CAIRO_STATUS_SUCCESS)
      return false;

  }

  bool DashStyle::NavBar (cairo_t* cr, nux::View::State state)
  {
    // sanity check
    if (cairo_status (cr) != CAIRO_STATUS_SUCCESS)
      return false;

  }

  bool DashStyle::Separator (cairo_t* cr, Orientation orientation)
  {
    // sanity check
    if (cairo_status (cr) != CAIRO_STATUS_SUCCESS)
      return;

  }

  bool ButtonBackground (cairo_t* cr, nux::View::State state)
  {
    // sanity check
    if (cairo_status (cr) != CAIRO_STATUS_SUCCESS)
      return;

  }

  void SetFGColor (float r, float g, float b, float a)
  {
  }

  void SetBGColor (float r, float g, float b, float a)
  {
  }

  void DashStyle::RoundedRect (cairo_t* cr)
  {
    // sanity check
    if (cairo_status (cr) != CAIRO_STATUS_SUCCESS)
      return;

  }

  void DashStyle::Blur (cairo_t* cr)
  {
    // sanity check
    if (cairo_status (cr) != CAIRO_STATUS_SUCCESS)
      return;

  }

  void DashStyle::Text (cairo_t* cr)
  {
    // sanity check
    if (cairo_status (cr) != CAIRO_STATUS_SUCCESS)
      return;

  }

  void DashStyle::Triangle (cairo_t*              cr,
                            Triangle::Orientation orientation,
                            nux::View::State      state)
  {
    // sanity check
    if (cairo_status (cr) != CAIRO_STATUS_SUCCESS)
      return;

  }

  void DashStyle::Star (cairo_t* cr, nux::View::State state)
  {
    // sanity check
    if (cairo_status (cr) != CAIRO_STATUS_SUCCESS)
      return;

    double outter[5][2] = {{0.0, 0.0},
                           {0.0, 0.0},
                           {0.0, 0.0},
                           {0.0, 0.0},
                           {0.0, 0.0}};
    double inner[5][2]  = {{0.0, 0.0},
                           {0.0, 0.0},
                           {0.0, 0.0},
                           {0.0, 0.0},
                           {0.0, 0.0}};
    double angle[5]     = {-90.0, -18.0, 54.0, 126.0, 198.0};
    double outterRadius = 1.0;
    double innerRadius  = 0.5;

    for (int i = 0; i < 5; i++)
    {
      outter[i][0] = outterRadius * cos (angle[i] * M_PI / 180.0);
      outter[i][1] = outterRadius * sin (angle[i] * M_PI / 180.0);
      inner[i][0]  = innerRadius * cos ((angle[i] + 36.0) * M_PI / 180.0);
      inner[i][1]  = innerRadius * sin ((angle[i] + 36.0) * M_PI / 180.0);
    }

    cairo_pattern_t* pattern = cairo_pattern_create_linear (0.0, 0.0, 10.0, 0.0);
    cairo_pattern_add_color_stop_rgba (pattern, 0.0,   1.0, 1.0, 1.0, 1.0);
    cairo_pattern_add_color_stop_rgba (pattern, 0.25,  1.0, 1.0, 1.0, 1.0);
    cairo_pattern_add_color_stop_rgba (pattern, 0.251, 1.0, 1.0, 1.0, 0.5);
    cairo_pattern_add_color_stop_rgba (pattern, 1.0,   1.0, 1.0, 1.0, 0.5);
    cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);
    cairo_set_source (cr, pattern);
    cairo_matrix_t matrix; // will die with the end of this scope
    cairo_matrix_init_rotate (&matrix, 45.0 * M_PI / 180.0);
    cairo_pattern_set_matrix (pattern, &matrix);
    //cairo_rectangle (cr, 100.0, 200.0, 75.0, 150.0);
    //cairo_fill (cr);

    cairo_translate (cr, 50.0, 50.0);
    cairo_save (cr);
    cairo_scale (cr, 25.0, 25.0);
    cairo_move_to (cr, outter[0][0], outter[0][1]);
    cairo_line_to (cr, inner[0][0], inner[0][1]);
    cairo_line_to (cr, outter[1][0], outter[1][1]);
    cairo_line_to (cr, inner[1][0], inner[1][1]);
    cairo_line_to (cr, outter[2][0], outter[2][1]);
    cairo_line_to (cr, inner[2][0], inner[2][1]);
    cairo_line_to (cr, outter[3][0], outter[3][1]);
    cairo_line_to (cr, inner[3][0], inner[3][1]);
    cairo_line_to (cr, outter[4][0], outter[4][1]);
    cairo_line_to (cr, inner[4][0], inner[4][1]);
    cairo_close_path (cr);
    cairo_restore (cr);

    cairo_translate (cr, 50.0 + 7.0, 0.0);
    cairo_save (cr);
    cairo_scale (cr, 25.0, 25.0);
    cairo_move_to (cr, outter[0][0], outter[0][1]);
    cairo_line_to (cr, inner[0][0], inner[0][1]);
    cairo_line_to (cr, outter[1][0], outter[1][1]);
    cairo_line_to (cr, inner[1][0], inner[1][1]);
    cairo_line_to (cr, outter[2][0], outter[2][1]);
    cairo_line_to (cr, inner[2][0], inner[2][1]);
    cairo_line_to (cr, outter[3][0], outter[3][1]);
    cairo_line_to (cr, inner[3][0], inner[3][1]);
    cairo_line_to (cr, outter[4][0], outter[4][1]);
    cairo_line_to (cr, inner[4][0], inner[4][1]);
    cairo_close_path (cr);
    cairo_restore (cr);

    cairo_translate (cr, 50.0 + 7.0, 0.0);
    cairo_save (cr);
    cairo_scale (cr, 25.0, 25.0);
    cairo_move_to (cr, outter[0][0], outter[0][1]);
    cairo_line_to (cr, inner[0][0], inner[0][1]);
    cairo_line_to (cr, outter[1][0], outter[1][1]);
    cairo_line_to (cr, inner[1][0], inner[1][1]);
    cairo_line_to (cr, outter[2][0], outter[2][1]);
    cairo_line_to (cr, inner[2][0], inner[2][1]);
    cairo_line_to (cr, outter[3][0], outter[3][1]);
    cairo_line_to (cr, inner[3][0], inner[3][1]);
    cairo_line_to (cr, outter[4][0], outter[4][1]);
    cairo_line_to (cr, inner[4][0], inner[4][1]);
    cairo_close_path (cr);
    cairo_restore (cr);

    cairo_translate (cr, 50.0 + 7.0, 0.0);
    cairo_save (cr);
    cairo_scale (cr, 25.0, 25.0);
    cairo_move_to (cr, outter[0][0], outter[0][1]);
    cairo_line_to (cr, inner[0][0], inner[0][1]);
    cairo_line_to (cr, outter[1][0], outter[1][1]);
    cairo_line_to (cr, inner[1][0], inner[1][1]);
    cairo_line_to (cr, outter[2][0], outter[2][1]);
    cairo_line_to (cr, inner[2][0], inner[2][1]);
    cairo_line_to (cr, outter[3][0], outter[3][1]);
    cairo_line_to (cr, inner[3][0], inner[3][1]);
    cairo_line_to (cr, outter[4][0], outter[4][1]);
    cairo_line_to (cr, inner[4][0], inner[4][1]);
    cairo_close_path (cr);
    cairo_restore (cr);

    cairo_translate (cr, 50.0 + 7.0, 0.0);
    cairo_save (cr);
    cairo_scale (cr, 25.0, 25.0);
    cairo_move_to (cr, outter[0][0], outter[0][1]);
    cairo_line_to (cr, inner[0][0], inner[0][1]);
    cairo_line_to (cr, outter[1][0], outter[1][1]);
    cairo_line_to (cr, inner[1][0], inner[1][1]);
    cairo_line_to (cr, outter[2][0], outter[2][1]);
    cairo_line_to (cr, inner[2][0], inner[2][1]);
    cairo_line_to (cr, outter[3][0], outter[3][1]);
    cairo_line_to (cr, inner[3][0], inner[3][1]);
    cairo_line_to (cr, outter[4][0], outter[4][1]);
    cairo_line_to (cr, inner[4][0], inner[4][1]);
    cairo_close_path (cr);
    cairo_restore (cr);

    //cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.75);
    //cairo_rectangle (cr, -(4.0 * 50.0 + 4.0 * 7.0), -30.0, 4.0 * 50.0 + 4.0 * 7.0, 55.0);
    //cairo_clip (cr);
    cairo_fill_preserve (cr);
    cairo_pattern_destroy (pattern);
    pattern = cairo_pattern_create_rgba (1.0, 1.0, 1.0, 1.0);
    cairo_set_source (cr, pattern);
    //cairo_reset_clip (cr);
    cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 1.0);
    cairo_stroke (cr);

    cairo_set_source_rgba (cr, 1.0, 0.0, 0.0, 0.5);
    cairo_rectangle (cr, -(4.0 * 50.0 + 4.0 * 7.0), -30.0, 4.0 * 50.0 + 4.0 * 7.0, 55.0);
    cairo_fill (cr);

  }
}
