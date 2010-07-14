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
    static const int SPACING = 10;
    static const int RANDOM_TEXT_WIDTH = 400;

    /* Properties */
    private PlaceEntry? active_entry = null;

    private PlaceSearchBarBackground bg;

    private PlaceSearchEntry       entry;
    private PlaceSearchSectionsBar sections;

    public PlaceSearchBar ()
    {
      Object (orientation:Ctk.Orientation.HORIZONTAL,
              homogeneous:false,
              spacing:8);
    }

    construct
    {
      padding = {
        SPACING * 2.0f,
        SPACING * 1.0f,
        SPACING * 1.0f,
        SPACING * 1.0f
      };

      entry = new PlaceSearchEntry ();
      pack (entry, true, true);
      entry.show ();
      entry.text_changed.connect (on_search_text_changed);

      sections = new PlaceSearchSectionsBar ();
      pack (sections, false, true);
      entry.show ();

      bg = new PlaceSearchBarBackground (entry);
      set_background (bg);
      bg.show ();
    }

    public void reset ()
    {
      entry.reset ();
    }

    private override void allocate (Clutter.ActorBox        box,
                                    Clutter.AllocationFlags flags)
    {
      float ex = entry.x;
      float ewidth = entry.width;

      base.allocate (box, flags);

      if (entry.x != ex || entry.width != ewidth)
        {
          /* After discussion with upstream Clutter guys, it seems like the
           * warning when resizing a CairoTexture is a bug in Clutter, however
           * the fix is not trivial, so it was suggested to use a 0-sec timeout.
           * We don't use an idle as it seems to have a lower priority and the
           * user will see a jump between states, the 0-sec timeout seems to be
           * dealt with immediately in the text iteration.
           */
          Timeout.add (0, bg.update_background);
        }
    }

    private override void get_preferred_height (float     for_width,
                                                out float min_height,
                                                out float nat_height)
    {
      float mheight, nheight;

      entry.get_preferred_height (RANDOM_TEXT_WIDTH, out mheight, out nheight);
      min_height = mheight + SPACING * 3;
      nat_height = nheight + SPACING * 3;
    }

    private void on_search_text_changed (string? text)
    {
      if (active_entry is PlaceEntry)
        {
          var hints = new HashTable<string, string> (str_hash, str_equal);
          active_entry.set_search (text, hints);
        }
    }

    /*
     * Public Methods
     */
    public void set_active_entry_view (PlaceEntry entry, int x)
    {
      active_entry = entry;
      bg.entry_position = x;
      sections.set_active_entry (entry);
    }
  }

  public class PlaceSearchBarBackground : Ctk.Bin
  {
    /* This is a full path right now as we get this asset from the assets
     * package that is not installable (easily) normally. Will change to
     * Config.DATADIR as soon as this is fixed in the other package.
     */
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

    private Clutter.CairoTexture texture;
    private Ctk.EffectGlow       glow;

    public PlaceSearchEntry search_entry { get; construct; }

    public PlaceSearchBarBackground (PlaceSearchEntry search_entry)
    {
      Object (search_entry:search_entry);
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

      texture = new Clutter.CairoTexture (10, 10);
      add_actor (texture);
      texture.show ();

      /* Enable once clutk bug is fixed */
      glow = new Ctk.EffectGlow ();
      glow.set_color ({ 255, 255, 255, 255 });
      glow.set_factor (1.0f);
      glow.set_margin (5);
      add_effect (glow);
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

          Timeout.add (0, update_background);
        }
    }

    public bool update_background ()
    {
      Cairo.Context cr;

      texture.set_surface_size (last_width, last_height);

      cr = texture.create ();

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
      var radius = 10;

      cr.move_to  (x, y + radius);
      cr.curve_to (x, y,
                   x, y,
                   x + radius, y);

      if (entry_position != 0)
        {
          cr.line_to (entry_position - y, y);
          cr.line_to (entry_position, y - y);
          cr.line_to (entry_position + y, y);
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

      /* Cut out the search entry */
      cr.set_operator (Cairo.Operator.CLEAR);

      x = (int)search_entry.x;
      y = (int)(search_entry.y) - 1;
      width = x + (int)search_entry.width;
      height = y + (int)search_entry.height +1;

      cr.move_to  (x, y + radius);
      cr.curve_to (x, y,
                   x, y,
                   x + radius, y);
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

      cr.fill_preserve ();

      cr.set_operator  (Cairo.Operator.OVER);
      cr.set_source_rgba (1.0f, 1.0f, 1.0f, 0.6f);
      cr.stroke ();

      glow.set_invalidate_effect_cache (true);

      return false;
    }
  }
}
