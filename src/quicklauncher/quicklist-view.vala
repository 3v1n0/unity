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
 * Authored by Gordon Allott <gord.allott@canonical.com>,
 *             Mirco "MacSlow" Müller <mirco.mueller@canonical.com>
 *
 */

namespace Unity.Quicklauncher
{
  const float LINE_WIDTH         = 0.083f;
  const float TEXT_HEIGHT        = 1.0f;
  const float MAX_TEXT_WIDTH     = 15.0f;
  const float GAP                = 0.25f;
  const float MARGIN             = 0.5f;
  const float BORDER             = 0.25f;
  const float CORNER_RADIUS      = 0.3f;
  const float ITEM_HEIGHT        = 2.0f;
  const float ITEM_CORNER_RADIUS = 0.3f;
  const float ANCHOR_HEIGHT      = 1.5f;
  const float ANCHOR_WIDTH       = 0.75f;

  // we subclass Ctk.MenuItem here because we need to adapt it's appearance
  public class QuicklistMenuItem : Ctk.MenuItem
  {
    Ctk.LayerActor item_background;
    int            old_width;
    int            old_height;
    string         old_label;

    private void
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

    private void
    _get_text_extents (out int width,
                       out int height)
    {
      Cairo.Surface dummy_surf = new Cairo.ImageSurface (Cairo.Format.A1, 1, 1);
      Cairo.Context dummy_cr = new Cairo.Context (dummy_surf);
      Pango.Layout layout = Pango.cairo_create_layout (dummy_cr);
      Gtk.Settings settings = Gtk.Settings.get_default ();
      string face = settings.gtk_font_name;
      Pango.FontDescription desc = Pango.FontDescription.from_string (face);

      desc.set_weight (Pango.Weight.NORMAL);
      layout.set_font_description (desc);
      layout.set_wrap (Pango.WrapMode.WORD_CHAR);
      layout.set_ellipsize (Pango.EllipsizeMode.END);
      layout.set_text (label, -1);
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

    private void _normal_mask (Cairo.Context cr,
                               int           w,
                               int           h,
                               string        label)
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
      string font_face = settings.gtk_font_name;
      Pango.FontDescription desc = Pango.FontDescription.from_string (font_face);
      desc.set_weight (Pango.Weight.NORMAL);
      layout.set_font_description (desc);
      layout.set_wrap (Pango.WrapMode.WORD_CHAR);
      layout.set_ellipsize (Pango.EllipsizeMode.END);
      layout.set_text (label, -1);
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
      _get_text_extents (out text_width, out text_height);
      cr.move_to (Ctk.em_to_pixel (MARGIN),
                  (float) (h - text_height) / 2.0f);

      Pango.cairo_show_layout (cr, layout);
    }

    private void _selected_mask (Cairo.Context cr,
                                 int           w,
                                 int           h,
                                 string        label)
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
                   Ctk.em_to_pixel (ITEM_CORNER_RADIUS),
                   w - 1.0f,
                   h - 1.0f);
      cr.fill ();

      // draw text
      Pango.Layout layout = Pango.cairo_create_layout (cr);
      Gtk.Settings settings = Gtk.Settings.get_default ();
      string font_face = settings.gtk_font_name;
      Pango.FontDescription desc = Pango.FontDescription.from_string (font_face);
      desc.set_weight (Pango.Weight.NORMAL);
      layout.set_font_description (desc);
      layout.set_wrap (Pango.WrapMode.WORD_CHAR);
      layout.set_ellipsize (Pango.EllipsizeMode.END);
      layout.set_text (label, -1);
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
      _get_text_extents (out text_width, out text_height);
      cr.move_to (Ctk.em_to_pixel (MARGIN),
                  (float) (h - text_height) / 2.0f);

      cr.set_source_rgba (0.0f, 0.0f, 0.0f, 0.0f);
      Pango.cairo_show_layout (cr, layout);
    }

    private override void
    paint ()
    {
      this.item_background.paint ();
    }

    public override void
    get_preferred_height (float     for_width,
                          out float min_height_p,
                          out float natural_height_p)
    {
      min_height_p = (float) Ctk.em_to_pixel (ITEM_HEIGHT);
      natural_height_p = min_height_p;
    }

    public override void
    get_preferred_width (float for_height,
                         out float min_width_p,
                         out float natural_width_p)
    {
      int width;
      int height;
      _get_text_extents (out width, out height);
      min_width_p = (float) width + (float) Ctk.em_to_pixel (2 * MARGIN);
      natural_width_p = min_width_p;
    }

    private override void
    allocate (Clutter.ActorBox        box,
              Clutter.AllocationFlags flags)
    {
      int w;
      int h;
      Clutter.Color white_color = Clutter.Color () {
        red   = 255,
        green = 255,
        blue  = 255,
        alpha = 255
      };

      base.allocate (box, flags);

      w = (int) (box.x2 - box.x1);
      h = (int) (box.y2 - box.y1);

      // exit early if the allocation-width/height didn't change, this is needed
      // because clutter triggers calling allocate even if nothing changed
      if ((old_width == w) && (old_height == h))
        return;

      // store the new width/height
      old_width  = w;
      old_height = h;

      // before creating a new CtkLayerActor make sure we don't leak any memory
      if (this.item_background is Ctk.LayerActor)
         this.item_background.unparent ();
      this.item_background = new Ctk.LayerActor (w, h);

      Ctk.Layer normal_layer = new Ctk.Layer (w,
                                              h,
                                              Ctk.LayerRepeatMode.NONE,
                                              Ctk.LayerRepeatMode.NONE);
      Ctk.Layer selected_layer = new Ctk.Layer (w,
                                                h,
                                                Ctk.LayerRepeatMode.NONE,
                                                Ctk.LayerRepeatMode.NONE);

      Cairo.Surface normal_surf = new Cairo.ImageSurface (Cairo.Format.ARGB32,
                                                          w,
                                                          h);
      Cairo.Surface selected_surf = new Cairo.ImageSurface (Cairo.Format.ARGB32,
                                                            w,
                                                            h);

      Cairo.Context normal_cr = new Cairo.Context (normal_surf);
      Cairo.Context selected_cr = new Cairo.Context (selected_surf);

      _normal_mask (normal_cr, w, h, this.get_label ());
      _selected_mask (selected_cr, w, h, this.get_label ());

      //normal_surf.write_to_png ("/tmp/normal_surf.png");
      //selected_surf.write_to_png ("/tmp/selected_surf.png");

      normal_layer.set_mask_from_surface (normal_surf);
      normal_layer.set_color (white_color);

      selected_layer.set_mask_from_surface (selected_surf);
      selected_layer.set_color (white_color);

      this.item_background.add_layer (normal_layer);
      this.item_background.add_layer (selected_layer);

      this.item_background.get_layer(0).set_enabled (true);
      this.item_background.get_layer(1).set_enabled (false);
      this.item_background.do_queue_redraw ();

      this.item_background.set_parent (this);
      this.item_background.map ();
      this.item_background.show ();
    }

    private bool _on_enter (Clutter.Event event)
    {
      this.item_background.get_layer(0).set_enabled (false);
      this.item_background.get_layer(1).set_enabled (true);
      this.item_background.do_queue_redraw ();
      return false;
    }

    private bool _on_leave (Clutter.Event event)
    {
      this.item_background.get_layer(0).set_enabled (true);
      this.item_background.get_layer(1).set_enabled (false);
      this.item_background.do_queue_redraw ();
      return false;
    }

    private void _on_label_changed ()
    {
      // if the contents of the label didn't really change exit early
      if (old_label == this.get_label ())
        return;

      old_label = this.get_label ();
    }

    public QuicklistMenuItem (string label)
    {
      Object (label:label);
    }

    construct
    {
      Ctk.Padding padding = Ctk.Padding () {
        left   = (int) Ctk.em_to_pixel (MARGIN),
        right  = (int) Ctk.em_to_pixel (MARGIN),
        top    = (int) Ctk.em_to_pixel (MARGIN),
        bottom = (int) Ctk.em_to_pixel (MARGIN)
      };
      this.set_padding (padding);

      this.notify["label"].connect (this._on_label_changed);
      this.enter_event.connect (this._on_enter);
      this.leave_event.connect (this._on_leave);

      old_width  = 0;
      old_height = 0;
      old_label  = "";
    }
  }

  // we call this instead of Ctk.Menu so you can alter this to look right
  public class QuicklistMenu : Ctk.Menu
  {
    Ctk.LayerActor ql_background;
    int            old_width;
    int            old_height;

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

    /* Commented out as it isn't used atm
    private void
    _debug_mask (Cairo.Context cr,
                 int           w,
                 int           h)
    {
      // clear context
      cr.set_operator (Cairo.Operator.CLEAR);
      cr.paint ();

      // setup correct drawing
      cr.set_operator (Cairo.Operator.SOURCE);
      cr.scale (1.0f, 1.0f);
      cr.set_line_width (Ctk.em_to_pixel (LINE_WIDTH));
      cr.set_source_rgba (1.0f, 1.0f, 1.0f, 1.0f);

      // draw actual rectangle
      cr.rectangle (0.0f, 0.0f, w, h);
      cr.fill ();
    }
    */
    private void
    _outline_mask (Cairo.Context cr,
                   int           w,
                   int           h,
                   float         anchor_y)
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
                          Ctk.em_to_pixel (BORDER + ANCHOR_WIDTH),
                          0.5f,
                          Ctk.em_to_pixel (CORNER_RADIUS),
                          (double) w - Ctk.em_to_pixel (BORDER + ANCHOR_WIDTH) - 0.5f,
                          (double) h - 1.0f,
                          Ctk.em_to_pixel (ANCHOR_WIDTH),
                          Ctk.em_to_pixel (ANCHOR_HEIGHT),
                          Ctk.em_to_pixel (BORDER),
                          anchor_y);
      cr.stroke ();
    }

    private void
    _fill_mask (Cairo.Context cr,
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
      _round_rect_anchor (cr,
                          1.0f,
                          Ctk.em_to_pixel (BORDER + ANCHOR_WIDTH),
                          0.5f,
                          Ctk.em_to_pixel (CORNER_RADIUS),
                          (double) w - Ctk.em_to_pixel (BORDER + ANCHOR_WIDTH) - 0.5f,
                          (double) h - 1.0f,
                          Ctk.em_to_pixel (ANCHOR_WIDTH),
                          Ctk.em_to_pixel (ANCHOR_HEIGHT),
                          Ctk.em_to_pixel (BORDER),
                          anchor_y);
      cr.fill ();
    }

    private void
    _negative_mask (Cairo.Context cr,
                    int           w,
                    int           h,
                    float         anchor_y)
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
                          Ctk.em_to_pixel (CORNER_RADIUS),
                          (double) w,
                          (double) h,
                          Ctk.em_to_pixel (ANCHOR_WIDTH),
                          Ctk.em_to_pixel (ANCHOR_HEIGHT),
                          Ctk.em_to_pixel (BORDER),
                          anchor_y);
      cr.fill ();
    }

    private void
    _dotted_bg (Cairo.Context cr,
                int           w,
                int           h,
                float         anchor_y)
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
                          Ctk.em_to_pixel (CORNER_RADIUS),
                          (double) w,
                          (double) h,
                          Ctk.em_to_pixel (ANCHOR_WIDTH),
                          Ctk.em_to_pixel (ANCHOR_HEIGHT),
                          Ctk.em_to_pixel (BORDER),
                          anchor_y);
      cr.fill ();
    }

    private void
    _highlight_bg (Cairo.Context cr,
                   int           w,
                   int           h,
                   float         anchor_y)
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
                          Ctk.em_to_pixel (CORNER_RADIUS),
                          (double) w,
                          (double) h,
                          Ctk.em_to_pixel (ANCHOR_WIDTH),
                          Ctk.em_to_pixel (ANCHOR_HEIGHT),
                          Ctk.em_to_pixel (BORDER),
                          anchor_y);
      cr.fill ();
    }

    private override void
    allocate (Clutter.ActorBox        box,
              Clutter.AllocationFlags flags)
    {
      int w;
      int h;
      float x;
      float y;
      float ax;
      float ay;
      float aw;
      float ah;
      float new_y;

      base.allocate (box, flags);
      w = (int) (box.x2 - box.x1);
      h = (int) (box.y2 - box.y1);

      // exit early if the allocation-width/height didn't change, this is needed
      // because clutter triggers calling allocate even if nothing changed
      if ((old_width == w) && (old_height == h))
        return;

      Ctk.Actor actor = this.get_attached_actor ();
      actor.get_position (out ax, out ay);
      actor.get_size (out aw, out ah);
      //stdout.printf ("--- attached-actor: %.1f/%.1f ---\n", ax, ay);
      this.get_position (out x, out y);
      //stdout.printf ("--- menu-actor: %.1f/%.1f ---\n", x, y);
      //stdout.printf ("--- y-diff: %.1f ---\n\n", y - ay);
      new_y = ah / 2.0f;

      // store the new width/height
      old_width  = w;
      old_height = h;

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
      /*Clutter.Color debug_color = Clutter.Color () {
        red   = 64,
        green = 64,
        blue  = 255,
        alpha = (uint8) (255.0f * 0.5f)
      };*/

      // before creating a new CtkLayerActor make sure we don't leak any memory
      if (this.ql_background is Ctk.LayerActor)
         this.ql_background.destroy ();
      this.ql_background = new Ctk.LayerActor (w, h);

      /* Commented out as it isn't used atm
      Ctk.Layer debug_layer = new Ctk.Layer (w,
                                             h,
                                             Ctk.LayerRepeatMode.NONE,
                                             Ctk.LayerRepeatMode.NONE);
       */

      Ctk.Layer outline_layer = new Ctk.Layer (w,
                                               h,
                                               Ctk.LayerRepeatMode.NONE,
                                               Ctk.LayerRepeatMode.NONE);
      Ctk.Layer fill_layer = new Ctk.Layer (w,
                                            h,
                                            Ctk.LayerRepeatMode.NONE,
                                            Ctk.LayerRepeatMode.NONE);
      Ctk.Layer highlight_layer = new Ctk.Layer (w,
                                                 h,
                                                 Ctk.LayerRepeatMode.NONE,
                                                 Ctk.LayerRepeatMode.NONE);
      Ctk.Layer dotted_layer = new Ctk.Layer (w,
                                              h,
                                              Ctk.LayerRepeatMode.NONE,
                                              Ctk.LayerRepeatMode.NONE);

      /*Ctk.Layer shadow_layer = new Ctk.Layer (w,
                                              h,
                                              Ctk.LayerRepeatMode.NONE,
                                              Ctk.LayerRepeatMode.NONE);*/

      /*Cairo.Surface debug_surf = new Cairo.ImageSurface (Cairo.Format.ARGB32,
                                                         w,
                                                         h);*/
      Cairo.Surface outline_surf = new Cairo.ImageSurface (Cairo.Format.ARGB32,
                                                           w,
                                                           h);
      Cairo.Surface fill_surf = new Cairo.ImageSurface (Cairo.Format.ARGB32,
                                                        w,
                                                        h);
      Cairo.Surface negative_surf = new Cairo.ImageSurface (Cairo.Format.ARGB32,
                                                            w,
                                                            h);
      Cairo.Surface highlight_surf = new Cairo.ImageSurface (Cairo.Format.ARGB32,
                                                            w,
                                                            h);
      Cairo.Surface dotted_surf = new Cairo.ImageSurface (Cairo.Format.ARGB32,
                                                          w,
                                                          h);

      //Cairo.Context debug_cr = new Cairo.Context (debug_surf);
      Cairo.Context outline_cr = new Cairo.Context (outline_surf);
      Cairo.Context fill_cr = new Cairo.Context (fill_surf);
      Cairo.Context negative_cr = new Cairo.Context (negative_surf);
      Cairo.Context highlight_cr = new Cairo.Context (highlight_surf);
      Cairo.Context dotted_cr = new Cairo.Context (dotted_surf);

      //_debug_mask (debug_cr, w, h);
      _outline_mask (outline_cr, w, h, new_y);
      _fill_mask (fill_cr, w, h, new_y);
      _negative_mask (negative_cr, w, h, new_y);
      _dotted_bg (dotted_cr, w, h, new_y);
      _highlight_bg (highlight_cr, w, h, new_y);

      //outline_surf.write_to_png ("/tmp/outline_surf.png");
      //fill_surf.write_to_png ("/tmp/fill_surf.png");
      //negative_surf.write_to_png ("/tmp/negative_surf.png");
      //highlight_surf.write_to_png ("/tmp/highlight_surf.png");
      //dotted_surf.write_to_png ("/tmp/dotted_surf.png");

      //shadow_layer.set_mask_from_surface (negative_surf);
      //shadow_layer.set_image_from_surface (shadow_surf);

      //debug_layer.set_mask_from_surface (debug_surf);
      //debug_layer.set_color (debug_color);

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
      //this.ql_background.add_layer (debug_layer);
      this.ql_background.add_layer (fill_layer);
      this.ql_background.add_layer (dotted_layer);
      this.ql_background.add_layer (highlight_layer);
      this.ql_background.add_layer (outline_layer);

      // important run-time optimization!
      this.ql_background.flatten ();

      this.set_background (this.ql_background);
    }
    
    construct
    {
      Ctk.Padding padding = Ctk.Padding () {
        left   = (int) Ctk.em_to_pixel (ANCHOR_WIDTH + 1.75f * BORDER),
        right  = (int) Ctk.em_to_pixel (BORDER),
        top    = (int) Ctk.em_to_pixel (BORDER),
        // FIXME: there's an issue with CtkMenu probably adding the spacing even
        // for the last child/menu-item although it should not do that, that's
        // why bottom is currently set to 0 instead of BORDER
        bottom = 0 // (int) Ctk.em_to_pixel (BORDER)
      };
      this.set_padding (padding);
      this.spacing = (int) Ctk.em_to_pixel (GAP);

      old_width  = 0;
      old_height = 0;
    }
  }
}
