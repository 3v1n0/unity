/*
 * Copyright (C) 2010 Canonical Ltd
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
 * Authored by Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */

namespace Unity.Places
{
  public class PlaceBar : Ctk.Box
  {
    /* Properties */
    public Shell      shell { get; construct; }
    public PlaceModel model { get; set construct; }

    private PlaceBarBackground bg;
    private Ctk.EffectGlow     glow;
    private PlaceEntryView     active_view = null;

    public PlaceBar (Shell shell, PlaceModel model)
    {
      Object (shell:shell,
              model:model,
              orientation:Ctk.Orientation.HORIZONTAL,
              homogeneous:false,
              spacing:0);
    }

    construct
    {
      bg = new PlaceBarBackground (shell);
      set_background (bg);
      bg.show ();

      /* Enable once clutk bug is fixed */
      glow = new Ctk.EffectGlow ();
      glow.set_color ({ 255, 255, 255, 255 });
      glow.set_factor (1.0f);
      glow.set_margin (5);
      add_effect (glow);

      foreach (Place place in model)
        {
          var view = new PlaceView (place);
          pack (view, false, true);
          view.show ();

          view.entry_activated.connect (on_entry_activated);
        }
    }

    private override void allocate (Clutter.ActorBox        box,
                                    Clutter.AllocationFlags flags)
    {
      padding = {
        0.0f,
        (float)shell.get_indicators_width (),
        0.0f,
        (float)shell.get_launcher_width_foobar ()};

      base.allocate (box, flags);
    }

    private override void get_preferred_height (float     for_width,
                                                out float min_height,
                                                out float nat_height)
    {
      min_height = 56.0f;
      nat_height = 56.0f;
    }

    private void on_entry_activated (PlaceView view, PlaceEntryView entry_view)
    {
      if (active_view is PlaceEntryView)
        {
          active_view.entry.active = false;
        }

      if (entry_view is PlaceEntryView)
        {
          active_view = entry_view;
          active_view.entry.active = true;
          bg.entry_position = (int)(view.x + entry_view.x);

          glow.set_invalidate_effect_cache (true);
        }
    }
  }

  public class PlaceBarBackground : Clutter.CairoTexture
  {
    public const string BG = "/usr/share/unity/dash_background.png";

    public Shell shell { get; construct; }

    private int last_width = 0;
    private int last_height = 0;

    Gdk.Pixbuf tile = null;

    private int _entry_position = 0;
    public int entry_position {
      get { return _entry_position; }
      set {
        if (_entry_position != value)
          {
            _entry_position = value;
            update_background ();
          }
      }
    }

    public PlaceBarBackground (Shell shell)
    {
      Object (shell:shell);
    }

    construct
    {
      try
        {
          /* I've loaded this in directly and not through theme due to use
           * not having a good loader for pixbufs from the theme right now
           */
          tile = new Gdk.Pixbuf.from_file (BG);
        }
      catch (Error e)
        {
          warning ("Unable to load dash background");
        }
    }

    private override void allocate (Clutter.ActorBox        box,
                                    Clutter.AllocationFlags flags)
    {
      int width = (int)(box.x2 - box.x1);
      int height = (int)(box.y2 - box.y1);

      base.allocate (box, flags);

      if (width != last_width || height != last_height)
        {
          last_width = width;
          last_height = height;

          Idle.add (update_background);
        }
    }

    private bool update_background ()
    {
      Cairo.Context cr;

      set_surface_size (last_width, last_height);

      cr = create ();

      cr.set_operator (Cairo.Operator.CLEAR);
      cr.paint ();

      cr.set_operator (Cairo.Operator.OVER);
      cr.set_line_width (1.5);
      cr.set_source_rgba (1.0, 1.0, 1.0, 0.0);

      cr.translate (-0.5, -0.5);

      /* Basic variables */
      var x = -1;
      var y = -1;
      var width = last_width + 2;
      var height = last_height;
      var radius = 13;
      var launcher_width = shell.get_launcher_width_foobar ();
      var panel_height = shell.get_panel_height_foobar ();
      var entry_width = PlaceEntryView.WIDTH;
      var top_padding = 3;

      /* Draw the top outline */
      cr.move_to (x, y);
      cr.line_to (width, y);
      cr.line_to (width, height);

      if (entry_position != 0)
        {
          /* This is when we have an active place entry */
          var x1 = entry_position;
          var x2 = entry_position + entry_width;

          cr.line_to  (x2 + radius, height);
          cr.curve_to (x2, height,
                       x2, height,
                       x2, height - radius);
          cr.line_to  (x2, radius + top_padding);
          cr.curve_to (x2, top_padding,
                       x2, top_padding,
                       x2 - radius, top_padding);
          cr.line_to  (x1 + radius, top_padding);
          cr.curve_to (x1, top_padding,
                       x1, top_padding,
                       x1, top_padding + radius);

          if (x1 < launcher_width + entry_width) /* first entry */
            {
              cr.line_to (launcher_width, panel_height);
            }
          else
            {
              cr.line_to  (x1, height - radius);
              cr.curve_to (x1, height,
                           x1, height,
                           x1-radius, height);
              cr.line_to  (launcher_width + radius, height);
              cr.curve_to (launcher_width, height,
                           launcher_width, height,
                           launcher_width, height - radius);
              cr.line_to  (launcher_width, panel_height);
            }
        }
      else
        {
          cr.line_to  (launcher_width + radius, height);
          cr.curve_to (launcher_width, height,
                       launcher_width, height,
                       launcher_width, height - radius);
          cr.line_to  (launcher_width, panel_height);
        }

      cr.line_to (x, panel_height);
      cr.close_path ();

      cr.stroke_preserve ();
      cr.clip_preserve ();

      /* Tile the background */
      if (tile is Gdk.Pixbuf)
        {
          Gdk.cairo_set_source_pixbuf (cr, tile, 0, 0);
          var pat = cr.get_source ();
          pat.set_extend (Cairo.Extend.REPEAT);
          cr.paint_with_alpha (0.25);
        }

      /* Add the outline */
      cr.reset_clip ();
      cr.set_source_rgba (1.0, 1.0, 1.0, 0.6);
      cr.stroke ();

      return false;
    }
  }
}

