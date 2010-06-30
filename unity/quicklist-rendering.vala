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
  const float CORNER_RADIUS_ABS      = 5.0f;
  const float SHADOW_SIZE            = 1.25f;
  const float ITEM_HEIGHT            = 2.0f;
  const float ITEM_CORNER_RADIUS     = 0.3f;
  const float ITEM_CORNER_RADIUS_ABS = 4.0f;
  const float ANCHOR_HEIGHT          = 1.5f;
  const float ANCHOR_HEIGHT_ABS      = 18.0f;
  const float ANCHOR_WIDTH           = 0.75f;
  const float ANCHOR_WIDTH_ABS       = 10.0f;

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
      Gtk.Settings settings = Gtk.Settings.get_default ();
      Pango.FontDescription desc = Pango.FontDescription.from_string (font);
      desc.set_weight (Pango.Weight.NORMAL);
      layout.set_font_description (desc);
      layout.set_wrap (Pango.WrapMode.WORD_CHAR);
      layout.set_ellipsize (Pango.EllipsizeMode.END);
      layout.set_text (text, -1);
      Pango.Context pango_context = layout.get_context ();
      Gdk.Screen screen = Gdk.Screen.get_default ();
      Pango.cairo_context_set_font_options (pango_context,
                                            screen.get_font_options ());
      Pango.cairo_context_set_resolution (pango_context,
                                          (float) settings.gtk_xft_dpi /
                                          (float) Pango.SCALE);
      layout.context_changed ();
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
      Gtk.Settings settings = Gtk.Settings.get_default ();
      Pango.FontDescription desc = Pango.FontDescription.from_string (font);
      desc.set_weight (Pango.Weight.NORMAL);
      layout.set_font_description (desc);
      layout.set_wrap (Pango.WrapMode.WORD_CHAR);
      layout.set_ellipsize (Pango.EllipsizeMode.END);
      layout.set_text (text, -1);
      Pango.Context pango_context = layout.get_context ();
      Gdk.Screen screen = Gdk.Screen.get_default ();
      Pango.cairo_context_set_font_options (pango_context,
                                            screen.get_font_options ());
      Pango.cairo_context_set_resolution (pango_context,
                                          (float) settings.gtk_xft_dpi /
                                          (float) Pango.SCALE);
      layout.context_changed ();

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
      Gtk.Settings settings = Gtk.Settings.get_default ();
      Pango.FontDescription desc = Pango.FontDescription.from_string (font);
      desc.set_weight (Pango.Weight.NORMAL);
      layout.set_font_description (desc);
      layout.set_wrap (Pango.WrapMode.WORD_CHAR);
      layout.set_ellipsize (Pango.EllipsizeMode.END);
      layout.set_text (text, -1);

      Pango.Context pango_context = layout.get_context ();
      Gdk.Screen screen = Gdk.Screen.get_default ();
      Pango.cairo_context_set_font_options (pango_context,
                                            screen.get_font_options ());
      Pango.cairo_context_set_resolution (pango_context,
                                          (float) settings.gtk_xft_dpi /
                                          (float) Pango.SCALE);
      layout.context_changed ();

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
    _setup (out Cairo.Surface surf,
            out Cairo.Context cr,
            bool              outline,
            int               width,
            int               height,
            bool              negative)
    {
      // create context
      if (outline)
        surf = new Cairo.ImageSurface (Cairo.Format.ARGB32, width, height);
      else
        surf = new Cairo.ImageSurface (Cairo.Format.A8, width, height);

      cr = new Cairo.Context (surf);

      // clear context
      cr.scale (1.0f, 1.0f);
      if (outline)
        {
          cr.set_source_rgba (0.0f, 0.0f, 0.0f, 0.0f);
          cr.set_operator (Cairo.Operator.CLEAR);
        }  
      else
        {
          cr.set_operator (Cairo.Operator.OVER);
          if (negative)
            cr.set_source_rgba (0.0f, 0.0f, 0.0f, 0.0f);
          else
            cr.set_source_rgba (1.0f, 1.0f, 1.0f, 1.0f);
        }
      cr.paint ();
    }

    private static void
    _draw (Cairo.Context cr,
           bool          outline,
           float         line_width,
           float*        rgba,
           bool          negative,
           bool          stroke)
    {
      // prepare drawing
      cr.set_operator (Cairo.Operator.SOURCE);

      // actually draw the mask
      if (outline)
        {
          cr.set_line_width (line_width);
          cr.set_source_rgba (rgba[0], rgba[1], rgba[2], rgba[3]);
        }
      else
        {
          if (negative)
            cr.set_source_rgba (1.0f, 1.0f, 1.0f, 1.0f);
          else
            cr.set_source_rgba (0.0f, 0.0f, 0.0f, 0.0f);
        }

      // stroke or fill?
      if (stroke)
        cr.stroke_preserve ();
      else
        cr.fill_preserve ();
    }

    private static void
    _finalize (Cairo.Context cr,
               bool          outline,
               float         line_width,
               float*        rgba,
               bool          negative,
               bool          stroke)
    {
      // prepare drawing
      cr.set_operator (Cairo.Operator.SOURCE);

      // actually draw the mask
      if (outline)
        {
          cr.set_line_width (line_width);
          cr.set_source_rgba (rgba[0], rgba[1], rgba[2], rgba[3]);
        }
      else
        {
          if (negative)
            cr.set_source_rgba (1.0f, 1.0f, 1.0f, 1.0f);
          else
            cr.set_source_rgba (0.0f, 0.0f, 0.0f, 0.0f);
        }

      // stroke or fill?
      if (stroke)
        cr.stroke ();
      else
        cr.fill ();
    }

    private static void
    _top_mask_path (Cairo.Context cr,
                    float         anchor_width,
                    int           width,
                    int           height,
                    float         radius,
                    bool          outline)
    {
      // create path
      cr.move_to (anchor_width + 0.5f, 0.0f);
      cr.line_to (anchor_width + 0.5f, (double) height - radius - 0.5f);
      cr.arc_negative (anchor_width + radius + 0.5f,
                       (double) height - radius - 0.5f,
                       radius,
                       180.0f * GLib.Math.PI / 180.0f,
                       90.0f * GLib.Math.PI / 180.0f);
      cr.line_to ((double) width - radius - 0.5f, (double) height - 0.5f);
      cr.arc_negative ((double) width - radius - 0.5f,
                       (double) height - radius - 0.5f,
                       radius,
                       90.0f * GLib.Math.PI / 180.0f,
                       0.0f * GLib.Math.PI / 180.0f);
      cr.line_to ((double) width - 0.5f, 0.0f);
      if (!outline)
        cr.close_path ();
    }

    private static void
    _dyn_mask_path (Cairo.Context cr,
                    float         anchor_width,
                    int           width,
                    int           height,
                    bool          outline)
    {
      // create path
      cr.move_to (anchor_width + 0.5f, 0.0f);
      cr.line_to (anchor_width + 0.5f, (double) height);
      if (outline)
        cr.move_to ((double) width - 0.5f, (double) height);
      else
        cr.line_to ((double) width - 0.5f, (double) height);
      cr.line_to ((double) width - 0.5f, 0.0f);
      if (!outline)
        cr.close_path ();
    }

    private static void
    _anchor_mask_path (Cairo.Context cr,
                       float         anchor_width,
                       float         anchor_height,
                       int           width,
                       int           height,
                       bool          outline)
    {
      // create path
      cr.move_to (anchor_width + 0.5f, 0.0f);
      cr.line_to (anchor_width + 0.5f,
                  ((double) height - anchor_height) / 2.0f);
      cr.line_to (0.5f,
                  (((double) height - anchor_height) + anchor_height) / 2.0f);
      cr.line_to (anchor_width + 0.5f,
                 (double) height - ((double) height - anchor_height) / 2.0f);
      cr.line_to (anchor_width + 0.5f, (double) height);
      if (outline)
        cr.move_to ((double) width - 0.5f, (double) height);
      else
        cr.line_to ((double) width - 0.5f, (double) height);
      cr.line_to ((double) width - 0.5f, 0.0f);
    }

    private static void
    _bottom_mask_path (Cairo.Context cr,
                       float         anchor_width,
                       int           width,
                       int           height,
                       float         radius,
                       bool          outline)
    {
      // create path
      cr.move_to (anchor_width + 0.5f, (double) height);
      cr.line_to (anchor_width + 0.5f, radius + 0.5f);
      cr.arc (anchor_width + radius + 0.5f,
              radius + 0.5f,
              radius,
              180.0f * GLib.Math.PI / 180.0f,
              270.0f * GLib.Math.PI / 180.0f);
      cr.line_to ((double) width - radius - 0.5f, 0.5f);
      cr.arc ((double) width - radius - 0.5f,
              radius + 0.5f,
              radius,
              270.0f * GLib.Math.PI / 180.0f,
              0.0f * GLib.Math.PI / 180.0f);
      cr.line_to ((double) width - 0.5f, (double) height);
      if (!outline)
        cr.close_path ();
    }

    private static void
    _mask (Cairo.Context cr)
    {
      cr.set_operator (Cairo.Operator.CLEAR);
      cr.fill_preserve ();
    }

    private static void
    _outline (Cairo.Context cr,
              float         line_width,
              float[]       rgba_line)
    {
      cr.set_operator (Cairo.Operator.SOURCE);
      cr.set_source_rgba (rgba_line[0],
                          rgba_line[1],
                          rgba_line[2],
                          rgba_line[3]);
      cr.set_line_width (line_width);
      cr.stroke ();
    }

    public static void
    outline_shadow_top (out Cairo.Surface surf,
                        int               width,
                        int               height,
                        float             anchor_width,
                        float             corner_radius,
                        uint              shadow_radius,
                        float[]           rgba_shadow,
                        float             line_width,
                        float[]           rgba_line)
    {
      Cairo.Context cr;

      _setup (out surf, out cr, true, width, height, false);
      _top_mask_path (cr, anchor_width, width, height, corner_radius, false);
      _draw (cr, true, line_width, rgba_shadow, false, false);
      Ctk.surface_blur (surf, shadow_radius);
      _mask (cr);
      _outline (cr, line_width, rgba_line);
    }

    public static void
    outline_shadow_dyn (out Cairo.Surface surf,
                        int               width,
                        int               height,
                        float             anchor_width,
                        uint              shadow_radius,
                        float[]           rgba_shadow,
                        float             line_width,
                        float[]           rgba_line)
    {
      Cairo.Context cr;

      _setup (out surf, out cr, true, width, height, false);
      _dyn_mask_path (cr, anchor_width, width, height, false);
      _draw (cr, true, line_width, rgba_shadow, false, false);
      Ctk.surface_blur (surf, shadow_radius);
      _mask (cr);
      cr.new_path ();
      _dyn_mask_path (cr, anchor_width, width, height, true);
      _outline (cr, line_width, rgba_line);
    }

    public static void
    outline_shadow_anchor (out Cairo.Surface surf,
                           int               width,
                           int               height,
                           float             anchor_width,
                           float             anchor_height,
                           uint              shadow_radius,
                           float[]           rgba_shadow,
                           float             line_width,
                           float[]           rgba_line)
    {
      Cairo.Context cr;

      _setup (out surf, out cr, true, width, height, false);
      _anchor_mask_path (cr, anchor_width, anchor_height, width, height, false);
      _draw (cr, true, line_width, rgba_shadow, false, false);
      Ctk.surface_blur (surf, shadow_radius);
      _mask (cr);
      cr.new_path ();
      _anchor_mask_path (cr, anchor_width, anchor_height, width, height, true);
      _outline (cr, line_width, rgba_line);
    }

    public static void
    outline_shadow_bottom (out Cairo.Surface surf,
                           int               width,
                           int               height,
                           float             anchor_width,
                           float             corner_radius,
                           uint              shadow_radius,
                           float[]           rgba_shadow,
                           float             line_width,
                           float[]           rgba_line)
    {
      Cairo.Context cr;

      _setup (out surf, out cr, true, width, height, false);
      _bottom_mask_path (cr, anchor_width, width, height, corner_radius, true);
      _draw (cr, true, line_width, rgba_shadow, false, false);
      Ctk.surface_blur (surf, shadow_radius);
      _mask (cr);
      _outline (cr, line_width, rgba_line);
    }

    public static void
    tint_dot_hl (out Cairo.Surface surf,
                 int               width,
                 int               height,
                 float             hl_x,
                 float             hl_y,
                 float             hl_size,
                 float[]           rgba_tint,
                 float[]           rgba_hl)
    {
      Cairo.Context cr;
      Cairo.Surface dots_surf;
      Cairo.Context dots_cr;
      Cairo.Pattern dots_pattern;
      Cairo.Pattern hl_pattern;

      // create normal context
      surf = new Cairo.ImageSurface (Cairo.Format.ARGB32, width, height);
      cr = new Cairo.Context (surf);

      // create context for dot-pattern
      dots_surf = new Cairo.ImageSurface (Cairo.Format.ARGB32, 4, 4);
      dots_cr = new Cairo.Context (dots_surf);

      // clear normal context
      cr.scale (1.0f, 1.0f);
      cr.set_source_rgba (0.0f, 0.0f, 0.0f, 0.0f);
      cr.set_operator (Cairo.Operator.CLEAR);
      cr.paint ();

      // prepare drawing for normal context
      cr.set_operator (Cairo.Operator.OVER);

      // create path in normal context
      cr.rectangle (0.0f, 0.0f, (double) width, (double) height);  

      // fill path of normal context with tint
      cr.set_source_rgba (rgba_tint[0],
                          rgba_tint[1],
                          rgba_tint[2],
                          rgba_tint[3]);
      cr.fill_preserve ();

      // create pattern in dot-context
      dots_cr.set_operator (Cairo.Operator.CLEAR);
      dots_cr.paint ();
      dots_cr.scale (1.0f, 1.0f);
      dots_cr.set_operator (Cairo.Operator.OVER);
      dots_cr.set_source_rgba (rgba_hl[0],
                               rgba_hl[1],
                               rgba_hl[2],
                               rgba_hl[3]);
      dots_cr.rectangle (0.0f, 0.0f, 1.0f, 1.0f);
      dots_cr.fill ();
      dots_cr.rectangle (2.0f, 2.0f, 1.0f, 1.0f);
      dots_cr.fill ();
      dots_pattern = new Cairo.Pattern.for_surface (dots_surf);

      // fill path of normal context with dot-pattern
      cr.set_operator (Cairo.Operator.OVER);
      cr.set_source (dots_pattern);
      dots_pattern.set_extend (Cairo.Extend.REPEAT);
      cr.fill_preserve ();

      // draw highlight
      cr.set_operator (Cairo.Operator.OVER);
      hl_pattern = new Cairo.Pattern.radial (hl_x,
                                             hl_y,
                                             0.0f,
                                             hl_x,
                                             hl_y,
                                             hl_size);
      hl_pattern.add_color_stop_rgba (0.0f,
                                      rgba_hl[0],
                                      rgba_hl[1],
                                      rgba_hl[2],
                                      rgba_hl[3]);
      hl_pattern.add_color_stop_rgba (1.0f, 1.0f, 1.0f, 1.0f, 0.0f);
      cr.set_source (hl_pattern);
      cr.fill ();
    }

    public static void
    top_mask (out Cairo.Surface surf,
              int               width,
              int               height,
              float             radius,
              float             anchor_width,
              bool              negative,
              bool              outline,
              float             line_width,
              float[]           rgba)
    {
      Cairo.Context cr;

      _setup (out surf, out cr, outline, width, height, negative);
      _top_mask_path (cr, anchor_width, width, height, radius, outline);
      _finalize (cr, outline, line_width, rgba, negative, outline);
    }

    public static void
    dyn_mask (out Cairo.Surface surf,
              int               width,
              int               height,
              float             anchor_width,
              bool              negative,
              bool              outline,
              float             line_width,
              float[]           rgba)
    {
      Cairo.Context cr;

      _setup (out surf, out cr, outline, width, height, negative);
      _dyn_mask_path (cr, anchor_width, width, height, outline);
      _finalize (cr, outline, line_width, rgba, negative, outline);
    }

    public static void
    anchor_mask (out Cairo.Surface surf,
                 int               width,
                 int               height,
                 float             anchor_width,
                 float             anchor_height,
                 bool              negative,
                 bool              outline,
                 float             line_width,
                 float[]           rgba)
    {
      Cairo.Context cr;

      _setup (out surf, out cr, outline, width, height, negative);
      _anchor_mask_path (cr,
                         anchor_width,
                         anchor_height,
                         width,
                         height,
                         outline);
      _finalize (cr, outline, line_width, rgba, negative, outline);
    }

    public static void
    bottom_mask (out Cairo.Surface surf,
                 int               width,
                 int               height,
                 float             radius,
                 float             anchor_width,
                 bool              negative,
                 bool              outline,
                 float             line_width,
                 float[]           rgba)
    {
      Cairo.Context cr;

      _setup (out surf, out cr, outline, width, height, negative);
      _bottom_mask_path (cr, anchor_width, width, height, radius, outline);
      _finalize (cr, outline, line_width, rgba, negative, outline);
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
