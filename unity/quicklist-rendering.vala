/*
 * Copyright (C) 2010 Canonical, Ltd.
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License
 * version 3.0 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License version 3.0 for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Authored by Mirco "MacSlow" Müller <mirco.mueller@canonical.com>
 *
 */

using Cairo;

namespace Unity.QuicklistRendering
{
  const float LINE_WIDTH             = 0.083f;
  const float LINE_WIDTH_ABS         = 1.5f;
  const float TEXT_HEIGHT            = 1.0f;
  const float MAX_TEXT_WIDTH         = 15.0f;
  const float GAP                    = 0.25f;
  const float MARGIN                 = 0.5f;
  const float BORDER                 = 0.25f;
  const float CORNER_RADIUS          = 0.3f;
  const float CORNER_RADIUS_ABS      = 8.0f;
  const float SHADOW_SIZE            = 1.25f;
  const float ITEM_HEIGHT            = 2.0f;
  const float ITEM_CORNER_RADIUS     = 0.3f;
  const float ITEM_CORNER_RADIUS_ABS = 6.0f;
  const float ANCHOR_HEIGHT          = 1.5f;
  const float ANCHOR_HEIGHT_ABS      = 14.0f;
  const float ANCHOR_WIDTH           = 0.75f;
  const float ANCHOR_WIDTH_ABS       = 8.0f;

  public class Seperator : GLib.Object
  {
    public static void
    fill_mask (Cairo.Context cr)
    {
      // clear context
      cr.set_operator (Cairo.Operator.CLEAR);
      cr.paint ();

      // fill whole context with opaque white
      cr.set_operator (Cairo.Operator.SOURCE);
      cr.scale (1.0f, 1.0f);
      cr.set_source_rgba (1.0f, 1.0f, 1.0f, 1.0f);
      cr.paint ();
    }

    public static void
    image_background (Cairo.Context cr,
                      int           w,
                      int           h)
    {
      // clear context
      cr.set_operator (Cairo.Operator.CLEAR);
      cr.paint ();

      // draw separator-line
      cr.scale (1.0f, 1.0f);
      cr.set_operator (Cairo.Operator.SOURCE);

      // force the separator line to be 1.5-pixels thick
      cr.set_line_width (1.5f);

      cr.set_source_rgba (1.0f, 1.0f, 1.0f, 0.75f);
      cr.set_line_cap (Cairo.LineCap.ROUND);

      // align to the pixel-grid
      float half_height = (float) h / 2.0f;
      float fract = half_height - (int) half_height;
      if (fract == 0.0f)
        half_height += 0.5f;
      cr.move_to (0.0f, half_height);
      cr.line_to ((float) w, half_height);

      cr.stroke ();
    }
  }

  public class Item : GLib.Object
  {
    private static void
    _round_rect (Cairo.Context cr,
                 double        aspect,        // aspect-ratio
                 double        x,             // top-left corner
                 double        y,             // top-left corner
                 double        corner_radius, // "size" of the corners
                 double        width,         // width of the rectangle
                 double        height)        // height of the rectangle
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

      // top-left, right of the corner
      cr.arc (x + radius,
              y + radius,
              radius,
              180.0f * GLib.Math.PI / 180.0f,
              270.0f * GLib.Math.PI / 180.0f);
    }

    public static void
    get_text_extents (string  font,
                      string  text,
                      out int width,
                      out int height)
    {
      Cairo.Surface surface = new Cairo.ImageSurface (Cairo.Format.A1, 1, 1);
      Cairo.Context cr = new Cairo.Context (surface);
      Pango.Layout layout = Pango.cairo_create_layout (cr);
      Pango.FontDescription desc = Pango.FontDescription.from_string (font);
      desc.set_weight (Pango.Weight.NORMAL);
      layout.set_font_description (desc);
      layout.set_wrap (Pango.WrapMode.WORD_CHAR);
      layout.set_ellipsize (Pango.EllipsizeMode.END);
      layout.set_text (text, -1);
      Pango.Rectangle log_rect;
      layout.get_extents (null, out log_rect);
      width  = log_rect.width / Pango.SCALE;
      height = log_rect.height / Pango.SCALE;
    }

    public static void
    normal_mask (Cairo.Context cr,
                 int           w,
                 int           h,
                 string        font,
                 string        text)
    {
      // clear context
      cr.set_operator (Cairo.Operator.CLEAR);
      cr.paint ();

      // setup correct filled-drawing
      cr.set_operator (Cairo.Operator.SOURCE);
      cr.scale (1.0f, 1.0f);
      cr.set_source_rgba (1.0f, 1.0f, 1.0f, 1.0f);

      Pango.Layout layout = Pango.cairo_create_layout (cr);
      Pango.FontDescription desc = Pango.FontDescription.from_string (font);
      desc.set_weight (Pango.Weight.NORMAL);
      layout.set_font_description (desc);
      layout.set_wrap (Pango.WrapMode.WORD_CHAR);
      layout.set_ellipsize (Pango.EllipsizeMode.END);
      layout.set_text (text, -1);

      int text_width;
      int text_height;
      get_text_extents (font, text, out text_width, out text_height);
      cr.move_to (Ctk.em_to_pixel (MARGIN),
                  (float) (h - text_height) / 2.0f);

      Pango.cairo_show_layout (cr, layout);
    }

    public static void
    selected_mask (Cairo.Context cr,
                   int           w,
                   int           h,
                   string        font,
                   string        text)
    {
      // clear context
      cr.set_operator (Cairo.Operator.CLEAR);
      cr.paint ();

      // setup correct filled-drawing
      cr.set_operator (Cairo.Operator.SOURCE);
      cr.scale (1.0f, 1.0f);
      cr.set_source_rgba (1.0f, 1.0f, 1.0f, 1.0f);

      // draw rounded rectangle
      _round_rect (cr,
                   1.0f,
                   0.5f,
                   0.5f,
                   ITEM_CORNER_RADIUS_ABS,
                   w - 1.0f,
                   h - 1.0f);
      cr.fill ();

      // draw text
      Pango.Layout layout = Pango.cairo_create_layout (cr);
      Pango.FontDescription desc = Pango.FontDescription.from_string (font);
      desc.set_weight (Pango.Weight.NORMAL);
      layout.set_font_description (desc);
      layout.set_wrap (Pango.WrapMode.WORD_CHAR);
      layout.set_ellipsize (Pango.EllipsizeMode.END);
      layout.set_text (text, -1);

      int text_width;
      int text_height;
      get_text_extents (font, text, out text_width, out text_height);
      cr.move_to (Ctk.em_to_pixel (MARGIN),
                  (float) (h - text_height) / 2.0f);

      cr.set_source_rgba (0.0f, 0.0f, 0.0f, 0.0f);
      Pango.cairo_show_layout (cr, layout);
    }
  }

  public class Menu : GLib.Object
  {
    private static double
    _align (double val)
    {
      double fract = val - (int) val;

      if (fract != 0.5f)
        return (double) ((int) val + 0.5f);
      else
        return val;
    }

    private static void
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

      double a = _align (x);
      double b = _align (x + radius);
      double c = _align (x + width);
      double d = _align (x + width - radius);
      double e = _align (y);
      double f = _align (y + radius);
      double g = _align (y + height);
      double h = _align (y + height - radius);

      double[] p0  = {b, e};
      double[] p1  = {d, e};
      double[] p2  = {c, f};
      double[] p3  = {c, h};
      double[] p4  = {d, g};
      double[] p5  = {b, g};
      double[] p6  = {a, h};
      double[] p7  = {_align (anchor_x + anchor_width), _align (anchor_y + anchor_height / 2.0f)};
      double[] p8  = {_align (anchor_x), _align (anchor_y)};
      double[] p9  = {_align (anchor_x + anchor_width), _align (anchor_y - anchor_height / 2.0f)};
      double[] p10 = {a, f};

      double[] q0 = {a, e};
      double[] q1 = {c, e};
      double[] q2 = {c, g};
      double[] q3 = {a, g};

      // top-left, right of the corner
      cr.move_to (p0[0], p0[1]);

      // top-right, left of the corner
      cr.line_to (p1[0], p1[1]);

      // top-right, below the corner
      cr.curve_to (q1[0] - radius * 0.45f,
                   q1[1],
                   q1[0],
                   q1[1] + radius * 0.45f,
                   p2[0],
                   p2[1]);

      // bottom-right, above the corner
      cr.line_to (p3[0], p3[1]);

      // bottom-right, left of the corner
      cr.curve_to (q2[0],
                   q2[1] - radius * 0.45f,
                   q2[0] - radius * 0.45f,
                   q2[1],
                   p4[0],
                   p4[1]);

      // bottom-left, right of the corner
      cr.line_to (p5[0], p5[1]);

      // bottom-left, above the corner
      cr.curve_to (q3[0] + radius * 0.45f,
                   q3[1],
                   q3[0],
                   q3[1] - radius * 0.45f,
                   p6[0],
                   p6[1]);

      // draw anchor-arrow
      cr.line_to (p7[0], p7[1]);
      cr.line_to (p8[0], p8[1]);
      cr.line_to (p9[0], p9[1]);

      // top-left, right of the corner
      cr.line_to (p10[0], p10[1]);
      cr.curve_to (q0[0],
                   q0[1] + radius * 0.45f,
                   q0[0] + radius * 0.45f,
                   q0[1],
                   p0[0],
                   p0[1]);
    }

    private static void
    _draw_mask (Cairo.Context cr,
                int           w,
                int           h,
                float         anchor_y)
    {
      _round_rect_anchor (cr,
                          1.0f,
                          ANCHOR_WIDTH_ABS + Ctk.em_to_pixel (SHADOW_SIZE),
                          Ctk.em_to_pixel (SHADOW_SIZE),
                          CORNER_RADIUS_ABS,
                          (double) w - (ANCHOR_WIDTH_ABS + Ctk.em_to_pixel (2 * SHADOW_SIZE)),
                          (double) h - Ctk.em_to_pixel (2 * SHADOW_SIZE),
                          ANCHOR_WIDTH_ABS,
                          ANCHOR_HEIGHT_ABS,
                          Ctk.em_to_pixel (SHADOW_SIZE),
                          anchor_y);
    }

    public static void
    full_mask (Cairo.Context cr,
               int           w,
               int           h,
               float         anchor_y)
    {
      // fill context
      cr.set_operator (Cairo.Operator.CLEAR);
      cr.paint ();
      cr.set_operator (Cairo.Operator.SOURCE);
      cr.set_source_rgba (1.0f, 1.0f, 1.0f, 1.0f);
      cr.paint ();
    }

    public static void
    fill_mask (Cairo.Context cr,
               int           w,
               int           h,
               float         anchor_y)
    {
      // clear context
      cr.set_operator (Cairo.Operator.CLEAR);
      cr.paint ();

      // setup correct filled-drawing
      cr.set_operator (Cairo.Operator.SOURCE);
      cr.scale (1.0f, 1.0f);
      cr.set_source_rgba (1.0f, 1.0f, 1.0f, 1.0f);

      // draw actual outline
      _draw_mask (cr, w, h, anchor_y);
      cr.fill ();
    }

    public static void
    background (Cairo.Context cr,
                int           w,
                int           h,
                float         anchor_y)
    {
      Cairo.Surface dots = new Cairo.ImageSurface (Cairo.Format.ARGB32, 4, 4);
      Cairo.Context cr_dots = new Cairo.Context (dots);
      Cairo.Pattern dot_pattern;
      Cairo.Pattern hl_pattern;

      // clear context
      cr.set_operator (Cairo.Operator.CLEAR);
      cr.paint ();

      // setup correct filled-drawing
      cr.set_operator (Cairo.Operator.OVER);
      cr.scale (1.0f, 1.0f);
      cr.set_source_rgba (0.0f, 0.0f, 0.0f, 0.48f);

      // draw drop-shadow
      _draw_mask (cr, w, h, anchor_y);
      cr.fill ();
      Ctk.surface_blur (cr.get_target (), (int) Ctk.em_to_pixel (SHADOW_SIZE/2));

      // clear inner part
      cr.set_operator (Cairo.Operator.CLEAR);
      _draw_mask (cr, w, h, anchor_y);
      cr.fill ();

      // draw fill
      cr.set_operator (Cairo.Operator.OVER);
      cr.scale (1.0f, 1.0f);
      cr.set_source_rgba (0.0f, 0.0f, 0.0f, 0.7f);
      _draw_mask (cr, w, h, anchor_y);
      cr.fill ();

      // draw dotted pattern
      cr_dots.set_operator (Cairo.Operator.CLEAR);
      cr_dots.paint ();
      cr_dots.scale (1.0f, 1.0f);
      cr_dots.set_operator (Cairo.Operator.OVER);
      cr_dots.set_source_rgba (1.0f, 1.0f, 1.0f, 0.15f);
      cr_dots.rectangle (0.0f, 0.0f, 1.0f, 1.0f);
      cr_dots.fill ();
      cr_dots.rectangle (2.0f, 2.0f, 1.0f, 1.0f);
      cr_dots.fill ();
      dot_pattern = new Cairo.Pattern.for_surface (dots);
      cr.set_operator (Cairo.Operator.OVER);
      cr.set_source (dot_pattern);
      dot_pattern.set_extend (Cairo.Extend.REPEAT);
      _draw_mask (cr, w, h, anchor_y);
      cr.fill ();

      // draw highlight
      cr.set_operator (Cairo.Operator.OVER);
      hl_pattern = new Cairo.Pattern.radial ((double) w / 2.0f,
                                             Ctk.em_to_pixel (BORDER) - 15.0f,
                                             0.0f,
                                             (double) w / 2.0f,
                                             Ctk.em_to_pixel (BORDER) - 15.0f,
                                             (double) w / 2.0f + 20.0f);
      hl_pattern.add_color_stop_rgba (0.0f, 1.0f, 1.0f, 1.0f, 0.65f);
      hl_pattern.add_color_stop_rgba (1.0f, 1.0f, 1.0f, 1.0f, 0.0f);
      cr.set_source (hl_pattern);
      _draw_mask (cr, w, h, anchor_y);
      cr.fill ();

      // draw outline
      cr.set_operator (Cairo.Operator.SOURCE);
      cr.set_line_width (LINE_WIDTH_ABS);
      cr.set_source_rgba (1.0f, 1.0f, 1.0f, 0.75f);
      _draw_mask (cr, w, h, anchor_y);
      cr.stroke ();
    }
  }
}
