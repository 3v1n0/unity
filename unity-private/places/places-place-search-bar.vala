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
  public class PlaceSearchBar : Ctk.Box
  {
    static const int SPACING = 8;

    /* Properties */
    private PlaceEntry? active_entry = null;

    private PlaceSearchBarBackground bg;
    private Ctk.EffectGlow     glow;

    public PlaceSearchBar ()
    {
      Object (orientation:Ctk.Orientation.HORIZONTAL,
              homogeneous:false,
              spacing:8);
    }

    construct
    {
      bg = new PlaceSearchBarBackground ();
      set_background (bg);
      bg.show ();

      /* Enable once clutk bug is fixed */
      glow = new Ctk.EffectGlow ();
      glow.set_color ({ 255, 255, 255, 255 });
      glow.set_factor (1.0f);
      glow.set_margin (5);
      //add_effect (glow);
    }


    private override void get_preferred_height (float     for_width,
                                                out float min_height,
                                                out float nat_height)
    {
      min_height = 40.0f;
      nat_height = 40.0f;
    }

    /*
     * Public Methods
     */
    public void set_active_entry_view (PlaceEntry entry, int x)
    {
      active_entry = entry;
      bg.entry_position = x;
    }
  }

  public class PlaceSearchBarBackground : Clutter.CairoTexture
  {
    public const string BG = "/usr/share/unity/dash_background.png";

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

    public PlaceSearchBarBackground ()
    {
      Object ();
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

      cr.translate (0.5, 0.5);

      /* Basic variables */
      var x = 0;
      var y = PlaceSearchBar.SPACING;
      var width = last_width -2;
      var height = last_height - 2;
      var radius = 13;

      //cr.rectangle (x, y, width, height);

      cr.move_to  (x, y + radius);
      cr.curve_to (x, y,
                   x, y,
                   x + radius, y);

      if (entry_position != 0)
        {
          cr.line_to (entry_position - radius, y);
          cr.line_to (entry_position, y - y);
          cr.line_to (entry_position + radius, y);
        }

      cr.line_to  (width - radius, y);
      cr.curve_to (width, y,
                   width, y,
                   width, y + radius);
      cr.line_to  (width, height - radius);
      cr.curve_to (width, height,
                   width, height,
                   width - radius, height);
      cr.line_to  (x + radius, height);
      cr.curve_to (x, height,
                   x, height,
                   x, height - radius);
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

