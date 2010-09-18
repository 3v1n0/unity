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
    static const int RANDOM_TEXT_WIDTH = 400;

    /* Properties */
    private PlaceEntry? active_entry = null;

    private PlaceSearchBarBackground bg;

    private PlaceSearchNavigation  navigation;
    public  PlaceSearchEntry       entry;
    public  PlaceSearchSectionsBar sections;
    public  PlaceSearchExtraAction extra_action;

    private bool _search_fail = false;
    public bool search_fail {
      get { return _search_fail; }
      set {
        if (_search_fail != value)
          {
            _search_fail = value;
            bg.search_empty = _search_fail;
            bg.update_background ();
          }
      }
    }

    public PlaceSearchBar ()
    {
      Object (orientation:Ctk.Orientation.HORIZONTAL,
              homogeneous:false,
              spacing:20);

      Testing.ObjectRegistry.get_default ().register ("UnityPlacesSearchBar",
                                                      this);
    }

    construct
    {
      padding = {
        SPACING * 2.0f,
        SPACING * 1.0f,
        SPACING * 1.0f,
        SPACING * 1.0f
      };

      navigation = new PlaceSearchNavigation ();
      pack (navigation, false, true);
      navigation.show ();

      entry = new PlaceSearchEntry ();
      pack (entry, false, true);
      entry.show ();
      entry.text_changed.connect (on_search_text_changed);

      sections = new PlaceSearchSectionsBar ();
      pack (sections, false, true);
      sections.show ();

      /* We need a dummy to be able to space the action label */
      var space = new Clutter.Rectangle.with_color ({255, 255, 255, 0});
      pack (space, true, true);
      space.show ();
      
      extra_action = new PlaceSearchExtraAction ();
      pack (extra_action, false, true);
      extra_action.hide (); /* hidden by default */
      extra_action.activated.connect (on_extra_action_activated);

      bg = new PlaceSearchBarBackground (navigation, entry);
      set_background (bg);
      bg.show ();
    }

    public void reset ()
    {
      search_fail = false;
      entry.reset ();
    }

    public void search (string text)
    {
      entry.text.text = text;
      on_search_text_changed (text);
    }

    public string get_search_text ()
    {
      return entry.text.text;
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
    
    private void on_extra_action_activated ()
    {
      if (active_entry is PlaceEntry)
        {
          /* The UnityExtraAction spec mandates that we send the special URI
           * "." when the extra action is triggered */
          ActivationStatus result = active_entry.parent.activate (".", "");
      
          switch (result)
          {
            case ActivationStatus.ACTIVATED_SHOW_DASH:
              break;
            case ActivationStatus.NOT_ACTIVATED:
            case ActivationStatus.ACTIVATED_HIDE_DASH:          
            case ActivationStatus.ACTIVATED_FALLBACK:
              global_shell.hide_unity ();
              break;
            default:
              warning ("Unexpected activation status: %u", result);
              break;
          }
        }
    }

    /*
     * Public Methods
     */
    public void set_active_entry_view (PlaceEntry entry, int x, uint section=0)
    {
      active_entry = entry;
      bg.entry_position = x;
      sections.set_active_entry (entry);
      if (section != 0)
        {
          Idle.add (() => {
            sections.set_active_section (section);
            return false;
          });
        }

      navigation.set_active_entry (entry);
      this.entry.set_active_entry (entry);
      this.entry.text.grab_key_focus ();
      
      if (entry.hints != null)
        {
          if (entry.hints["UnityExtraAction"] != null)
            {
              extra_action.set_icon_from_gicon_string (
                                               entry.hints["UnityExtraAction"]);
              extra_action.show ();
            }
          else
            extra_action.hide ();
        }
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

    public bool search_empty { get; set; }

    public PlaceSearchNavigation navigation { get; construct; }
    public PlaceSearchEntry search_entry { get; construct; }

    public PlaceSearchBarBackground (PlaceSearchNavigation nav,
                                     PlaceSearchEntry search_entry)
    {
      Object (navigation:nav, search_entry:search_entry);
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

      search_empty = false;
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
      cr.set_line_width (1.0);
      cr.set_source_rgba (1.0, 1.0, 1.0, 0.0);

      cr.translate (0.5, 0.5);

      /* Basic variables */
      var x = 0;
      var y = PlaceSearchBar.SPACING;
      var width = last_width -2;
      var height = last_height - 2;
      var radius = 7;

      /* Text background */
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

      /* Add the arrow surround */
      x = (int)navigation.x;
      y = (int)navigation.y - 1;
      width = x + (int)navigation.width;
      height = y + (int)navigation.height;

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

      cr.set_source_rgba (1.0f, 1.0f, 1.0f, 0.05f);
      cr.fill_preserve ();

      cr.set_source_rgba (1.0f, 1.0f, 1.0f, 0.6f);
      cr.stroke ();

      /* Arrow separator */
      cr.rectangle (x + ((width-x)/2), y+1, 1, height - y - 1);
      cr.fill ();

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
      
      if (search_empty)
        {
          cr.set_source_rgba (1.0f, 0.0f, 0.0f, 0.3f);
          cr.fill_preserve ();
        }

      cr.set_source_rgba (1.0f, 1.0f, 1.0f, 0.6f);
      cr.stroke ();

      glow.set_invalidate_effect_cache (true);

      return false;
    }
  }
}
