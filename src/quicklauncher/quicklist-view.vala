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

  const int TMP_BG_WIDTH         = 200;
  const int TMP_BG_HEIGHT        = 250;

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

    private void
    _dotted_bg (Cairo.Context cr,
                int           w,
                int           h,
                int           item)
    {
      Cairo.Surface dots = new Cairo.ImageSurface (Cairo.Format.ARGB32, 4, 4);
      Cairo.Context cr_dots = new Cairo.Context (dots);

      // create dots in surface
      cr_dots.set_operator (Cairo.Operator.CLEAR);
      cr_dots.paint ();
      cr_dots.scale (1.0f, 1.0f);
      cr_dots.set_operator (Cairo.Operator.SOURCE);
      cr_dots.set_source_rgba (1.0f, 1.0f, 1.0f, 1.0f);
      cr_dots.rectangle (0.0f, 0.0f, 1.0f, 1.0f);
      cr_dots.fill ();
      cr_dots.rectangle (2.0f, 2.0f, 1.0f, 1.0f);
      cr_dots.fill ();

      // clear context
      cr.set_operator (Cairo.Operator.CLEAR);
      cr.paint ();

      // create pattern from dot-surface
      Cairo.Pattern pattern = new Cairo.Pattern.for_surface (dots);

      // setup correct filled-drawing
      cr.set_operator (Cairo.Operator.SOURCE);
      cr.scale (1.0f, 1.0f);
      cr.set_source (pattern);
      pattern.set_extend (Cairo.Extend.REPEAT);

      // fill masked area with the dotted pattern
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
    _highlight_bg (Cairo.Context cr,
                   int           w,
                   int           h,
                   int           item)
    {
      Cairo.Pattern pattern;

      // clear context
      cr.set_operator (Cairo.Operator.CLEAR);
      cr.paint ();

      // setup correct filled-drawing
      cr.set_operator (Cairo.Operator.SOURCE);
      cr.scale (1.0f, 1.0f);
      cr.set_source_rgba (1.0f, 1.0f, 1.0f, 1.0f);

      // setup radial gradient, acting as the hightlight, width defines diameter
      pattern = new Cairo.Pattern.radial ((double) w / 2.0f,
                                          Ctk.em_to_pixel (BORDER),
                                          0.0f,
                                          (double) w / 2.0f,
                                          Ctk.em_to_pixel (BORDER),
                                          (double) w / 2.0f);
      pattern.add_color_stop_rgba (0.0f, 1.0f, 1.0f, 1.0f, 0.5f);
      pattern.add_color_stop_rgba (1.0f, 1.0f, 1.0f, 1.0f, 0.0f);
      cr.set_source (pattern);

      // fill masked area with radial gradient "highlight"
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
      Ctk.Layer highlight_layer = new Ctk.Layer (TMP_BG_WIDTH,
                                                 TMP_BG_HEIGHT,
                                                 Ctk.LayerRepeatMode.NONE,
                                                 Ctk.LayerRepeatMode.NONE);
      Ctk.Layer dotted_layer = new Ctk.Layer (TMP_BG_WIDTH,
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
      Cairo.Surface highlight_surf = new Cairo.ImageSurface (Cairo.Format.ARGB32,
                                                            TMP_BG_WIDTH,
                                                            TMP_BG_HEIGHT);
      Cairo.Surface dotted_surf = new Cairo.ImageSurface (Cairo.Format.ARGB32,
                                                          TMP_BG_WIDTH,
                                                          TMP_BG_HEIGHT);

      Cairo.Context outline_cr = new Cairo.Context (outline_surf);
      Cairo.Context fill_cr = new Cairo.Context (fill_surf);
      Cairo.Context negative_cr = new Cairo.Context (negative_surf);
      Cairo.Context highlight_cr = new Cairo.Context (highlight_surf);
      Cairo.Context dotted_cr = new Cairo.Context (dotted_surf);

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

      _dotted_bg (dotted_cr,
                  TMP_BG_WIDTH,
                  TMP_BG_HEIGHT,
                  2);

      _highlight_bg (highlight_cr,
                     TMP_BG_WIDTH,
                     TMP_BG_HEIGHT,
                     2);

      //outline_surf.write_to_png ("/tmp/outline_surf.png");
      //fill_surf.write_to_png ("/tmp/fill_surf.png");
      //negative_surf.write_to_png ("/tmp/negative_surf.png");
      //highlight_surf.write_to_png ("/tmp/highlight_surf.png");
      //dotted_surf.write_to_png ("/tmp/dotted_surf.png");

      //shadow_layer.set_mask_from_surface (negative_surf);
      //shadow_layer.set_image_from_surface (shadow_surf);

      fill_layer.set_mask_from_surface (fill_surf);
      fill_layer.set_color (fill_color);

      dotted_layer.set_mask_from_surface (fill_surf);
      dotted_layer.set_image_from_surface (dotted_surf);
      dotted_layer.opacity = (uint8) (255.0f * 0.15f);

      highlight_layer.set_mask_from_surface (fill_surf);
      highlight_layer.set_image_from_surface (highlight_surf);
      highlight_layer.opacity = 128;

      outline_layer.set_mask_from_surface (outline_surf);
      outline_layer.set_color (outline_color);

      // order is important here... don't mess around!
      //this.ql_background.add_layer (shadow_layer);
      //this.ql_background.add_layer (blurred_layer);
      this.ql_background.add_layer (fill_layer);
      this.ql_background.add_layer (dotted_layer);
      this.ql_background.add_layer (highlight_layer);
      this.ql_background.add_layer (outline_layer);

      // important run-time optimization!
      this.ql_background.flatten ();

      this.set_background (this.ql_background);
    }
  }
}
