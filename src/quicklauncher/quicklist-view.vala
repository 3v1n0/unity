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
  const float LINE_WIDTH         = 0.083f;
  const float TEXT_HEIGHT        = 1.0f;
  const float MAX_TEXT_WIDTH     = 15.0f;
  const float GAP                = 0.5f;
  const float MARGIN             = 0.5f;
  const float BORDER             = 0.25f;
  const float CORNER_RADIUS      = 0.5f;
  const float ITEM_HEIGHT        = 2.0f;
  const float ITEM_CORNER_RADIUS = 0.16f;
  const float ANCHOR_HEIGHT      = 2.0f;
  const float ANCHOR_WIDTH       = 1.0f;

  const int TMP_BG_WIDTH         = 100;
  const int TMP_BG_HEIGHT        = 200;

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
    _outline_mask (Cairo.Context cr,
                   int           w,
                   int           h,
                   int           item)
    {
      // clear context
      cr.set_operator (Cairo.Operator.CLEAR);
      cr.paint ();

      // setup correct line-drawing
      cr.set_operator (Cairo.Operator.SOURCE);
      cr.scale (1.0f, 1.0f);
      cr.set_line_width (Ctk.em_to_pixel (LINE_WIDTH));
      cr.set_source_rgba (1.0f, 1.0f, 1.0f, 1.0f);

      // draw actual outline
      _round_rect_anchor (cr,
                          1.0f,
                          Ctk.em_to_pixel (BORDER) +
                          Ctk.em_to_pixel (ANCHOR_WIDTH),
                          Ctk.em_to_pixel (BORDER),
                          Ctk.em_to_pixel (BORDER),
                          (double) w,
                          (double) h, // items * Ctk.em_to_pixel (ITEM_HEIGHT)
                          Ctk.em_to_pixel (ANCHOR_WIDTH),
                          Ctk.em_to_pixel (ANCHOR_HEIGHT),
                          Ctk.em_to_pixel (BORDER),
                          item * Ctk.em_to_pixel (ITEM_HEIGHT) -
                          Ctk.em_to_pixel (ITEM_HEIGHT) / 2.0f);
      cr.stroke ();
    }

    private void
    _fill_mask (Cairo.Context cr,
                int           w,
                int           h,
                int           item)
    {
      // clear context
      cr.set_operator (Cairo.Operator.CLEAR);
      cr.paint ();

      // setup correct filled-drawing
      cr.set_operator (Cairo.Operator.SOURCE);
      cr.scale (1.0f, 1.0f);
      cr.set_source_rgba (1.0f, 1.0f, 1.0f, 1.0f);

      // draw actual outline
      _round_rect_anchor (cr,
                          1.0f,
                          Ctk.em_to_pixel (BORDER) +
                          Ctk.em_to_pixel (ANCHOR_WIDTH),
                          Ctk.em_to_pixel (BORDER),
                          Ctk.em_to_pixel (BORDER),
                          (double) w,
                          (double) h, // items * Ctk.em_to_pixel (ITEM_HEIGHT)
                          Ctk.em_to_pixel (ANCHOR_WIDTH),
                          Ctk.em_to_pixel (ANCHOR_HEIGHT),
                          Ctk.em_to_pixel (BORDER),
                          item * Ctk.em_to_pixel (ITEM_HEIGHT) -
                          Ctk.em_to_pixel (ITEM_HEIGHT) / 2.0f);
      cr.fill ();
    }

    private void
    _negative_mask (Cairo.Context cr,
                    int           w,
                    int           h,
                    int           item)
    {
      // clear context
      cr.set_operator (Cairo.Operator.SOURCE);
      cr.set_source_rgba (1.0f, 1.0f, 1.0f, 1.0f);
      cr.paint ();

      // setup correct mask-drawing
      cr.set_operator (Cairo.Operator.CLEAR);
      cr.scale (1.0f, 1.0f);

      // draw actual outline
      _round_rect_anchor (cr,
                          1.0f,
                          Ctk.em_to_pixel (BORDER) +
                          Ctk.em_to_pixel (ANCHOR_WIDTH),
                          Ctk.em_to_pixel (BORDER),
                          Ctk.em_to_pixel (BORDER),
                          (double) w,
                          (double) h, // items * Ctk.em_to_pixel (ITEM_HEIGHT)
                          Ctk.em_to_pixel (ANCHOR_WIDTH),
                          Ctk.em_to_pixel (ANCHOR_HEIGHT),
                          Ctk.em_to_pixel (BORDER),
                          item * Ctk.em_to_pixel (ITEM_HEIGHT) -
                          Ctk.em_to_pixel (ITEM_HEIGHT) / 2.0f);
      cr.fill ();
    }

    /*private void
    _dotted_bg (Cairo.Context cr)
    {
    }*/

    /*private void
    _highlight_bg (Cairo.Context cr)
    {
    }*/

    Ctk.LayerActor ql_background;

    construct
    {
      Clutter.Color outline_color = Clutter.Color () {
        red   = 255,
        green = 255,
        blue  = 255,
        alpha = 255
      };
      Clutter.Color fill_color = Clutter.Color () {
        red   = 0,
        green = 0,
        blue  = 0,
        alpha = (uint8) (255.0f * 0.3f)
      };

      this.ql_background = new Ctk.LayerActor (TMP_BG_WIDTH, TMP_BG_HEIGHT);

      Ctk.Layer outline_layer = new Ctk.Layer (TMP_BG_WIDTH,
                                               TMP_BG_HEIGHT,
                                               Ctk.LayerRepeatMode.NONE,
                                               Ctk.LayerRepeatMode.NONE);
      Ctk.Layer fill_layer = new Ctk.Layer (TMP_BG_WIDTH,
                                            TMP_BG_HEIGHT,
                                            Ctk.LayerRepeatMode.NONE,
                                            Ctk.LayerRepeatMode.NONE);

      /*Ctk.Layer shadow_layer = new Ctk.Layer (TMP_BG_WIDTH,
                                              TMP_BG_HEIGHT,
                                              Ctk.LayerRepeatMode.NONE,
                                              Ctk.LayerRepeatMode.NONE);*/

      Cairo.Surface outline_surf = new Cairo.ImageSurface (Cairo.Format.ARGB32,
                                                           TMP_BG_WIDTH,
                                                           TMP_BG_HEIGHT);
      Cairo.Surface fill_surf = new Cairo.ImageSurface (Cairo.Format.ARGB32,
                                                        TMP_BG_WIDTH,
                                                        TMP_BG_HEIGHT);
      Cairo.Surface negative_surf = new Cairo.ImageSurface (Cairo.Format.ARGB32,
                                                            TMP_BG_WIDTH,
                                                            TMP_BG_HEIGHT);

      Cairo.Context outline_cr = new Cairo.Context (outline_surf);
      Cairo.Context fill_cr = new Cairo.Context (fill_surf);
      Cairo.Context negative_cr = new Cairo.Context (negative_surf);

      _outline_mask (outline_cr,
                     TMP_BG_WIDTH,
                     TMP_BG_HEIGHT,
                     2);
      _fill_mask (fill_cr,
                  TMP_BG_WIDTH,
                  TMP_BG_HEIGHT,
                  2);
      _negative_mask (negative_cr,
                      TMP_BG_WIDTH,
                      TMP_BG_HEIGHT,
                      2);

      //_dotted_bg (cr);
      //_highlight_bg (cr);
      //outline_surf.write_to_png ("/tmp/outline_surf.png");
      //fill_surf.write_to_png ("/tmp/fill_surf.png");
      //negative_surf.write_to_png ("/tmp/negative_surf.png");

      //shadow_layer.set_mask_from_surface (negative_surf);
      //shadow_layer.set_image_from_surface (shadow_surf);

      fill_layer.set_mask_from_surface (fill_surf);
      fill_layer.set_color (fill_color);

      outline_layer.set_mask_from_surface (outline_surf);
      outline_layer.set_color (outline_color);

      // order is important here... don't mess around!
      this.ql_background.add_layer (fill_layer);
      this.ql_background.add_layer (outline_layer);

      this.set_background (this.ql_background);
    }
  }
}
