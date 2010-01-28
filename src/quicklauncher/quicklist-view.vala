/* -*- Mode: vala; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/*
 * Copyright (C) 2009 Canonical Ltd
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
 * Authored by Gordon Allott <gord.allott@canonical.com>
 *
 */

namespace Unity.Quicklauncher
{

  /* we call this instead of Ctk.Menu so you can alter this to look right */
  public class QuicklistMenu : Ctk.Menu
  {
    private void
    _round_rect_anchor (Cairo.Context cr,
                        double        aspect,        // aspect-ratio
                        double        x,             // top-left corner
                        double        y,             // top-left corner
                        double        corner_radius, // "size" of the corners
                        double        width,         // width of the rectangle
                        double        height,        // height of the rectangle
                        double        anchor_width,  // width of anchor
                        double        anchor_height, // height of anchor
                        double        anchor_x,      // x of anchor
                        double        anchor_y)      // y of anchor
    {
      double radius = corner_radius / aspect;

      // top-left, right of the corner
      cr.move_to (x + radius, y);

      // top-right, left of the corner
      cr.line_to (x + width - radius, y);

      // top-right, below the corner
      cr.arc (x + width - radius,
              y + radius,
              radius,
              -90.0f * GLib.Math.PI / 180.0f,
              0.0f * GLib.Math.PI / 180.0f);

      // bottom-right, above the corner
      cr.line_to (x + width, y + height - radius);

      // bottom-right, left of the corner
      cr.arc (x + width - radius,
              y + height - radius,
              radius,
              0.0f * GLib.Math.PI / 180.0f,
              90.0f * GLib.Math.PI / 180.0f);

      // bottom-left, right of the corner
      cr.line_to (x + radius, y + height);

      // bottom-left, above the corner
      cr.arc (x + radius,
              y + height - radius,
              radius,
              90.0f * GLib.Math.PI / 180.0f,
              180.0f * GLib.Math.PI / 180.0f);

      // bottom vertex of anchor
      cr.line_to (anchor_x + anchor_width, anchor_y + anchor_height / 2.0f);

      // middle vertex of anchor
      cr.line_to (anchor_x, anchor_y);

      // top vertex of anchor
      cr.line_to (anchor_x + anchor_width, anchor_y - anchor_height / 2.0f);

      // top-left, right of the corner
      cr.arc (x + radius,
              y + radius,
              radius,
              180.0f * GLib.Math.PI / 180.0f,
              270.0f * GLib.Math.PI / 180.0f);
    }

    private void
    _outline_mask (Cairo.Context cr)
    {
    }

    private void
    _fill_mask (Cairo.Context cr)
    {
    }

    private void
    _negative_mask (Cairo.Context cr)
    {
    }

    private void
    _dotted_bg (Cairo.Context cr)
    {
    }

    private void
    _highlight_bg (Cairo.Context cr)
    {
    }

    //Ctk.LayerActor ql_background2;
    Clutter.Rectangle ql_background;

    construct
    {
      Clutter.Color color = Clutter.Color () {
        red = 0x20,
        green = 0x30,
        blue = 0x40,
        alpha = 0xff
      };
      this.ql_background = new Clutter.Rectangle.with_color (color);

      Cairo.Surface surf = new Cairo.ImageSurface (Cairo.Format.A1, 1, 1);
      Cairo.Context cr = new Cairo.Context (surf);
      _outline_mask (cr);
      _fill_mask (cr);
      _negative_mask (cr);
      _dotted_bg (cr);
      _highlight_bg (cr);
      _round_rect_anchor (cr, 1.0f, 0.0f, 0.0f, 0.1f, 2.0f,
                        2.0f, 0.5f, 1.0f, 0.0f, 0.0f);

      this.set_background (this.ql_background);
      Ctk.Padding padding = Ctk.Padding () {
        left = 6,
        right = 6,
        top = 6,
        bottom = 6
      };
      this.set_padding (padding);
    }
  }
}
