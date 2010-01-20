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
 * Authored by Mirco "MacSlow" Müller <mirco.mueller@canonical.com>
 *             Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */

namespace Unity.Places.Bar
{
  const float PANEL_HEIGHT    = 24;
  const int   ICON_SIZE       = 48;
  const float ICON_VIEW_WIDTH = 80.0f;
  const float ICON_VIEW_Y1    = 8.0f;
  const float QL_PAD          = 12.0f;

  const string TRASH_FILE = Unity.PKGDATADIR + "/trash.png";

  public class View : Ctk.Box
  {
    public Places.Model model { get; construct; }

    private Gee.ArrayList<PlaceIcon> places_icons;
    private PlaceIcon                trash_icon;

    private PlacesVSeparator separator;
    private PlacesBackground bg;

    public View (Places.Model model)
    {
      Ctk.EffectGlow         glow;
      Clutter.Color          white = {255, 255, 255, 255};

      Object (model:model);

      /* We do our own allocation, but this doesn't hurt */
      this.homogeneous  = false;
      this.orientation  = Ctk.Orientation.HORIZONTAL;

      /* The background of the entire places bar */
      this.bg = new PlacesBackground ();
      this.bg.set_parent (this);
      this.bg.show ();

      /* This'll be populated in an idle by the model */
      this.places_icons = new Gee.ArrayList<PlaceIcon> ();
      this.model.place_added.connect (this.on_place_added);

      /* Create trash icon */
      this.trash_icon = new PlaceIcon (ICON_SIZE,
                                       "Trash",
                                       TRASH_FILE,
                                       "Your piece of waste");
      glow = new Ctk.EffectGlow ();
      glow.set_color (white);
      glow.set_factor (1.0f);
      glow.set_margin (6);
      this.trash_icon.add_effect (glow);

      this.pack (this.trash_icon, false, false);
      this.trash_icon.button_release_event.connect (this.on_button_release);

      /* Create the separator */
      this.separator = new PlacesVSeparator ();
      this.pack (this.separator, false, false);

      this.show_all ();
    }

    private void on_place_added (Place place)
    {
      Clutter.Color white = {255, 255, 255, 255};

      var icon = new PlaceIcon.from_place (ICON_SIZE, place);
      this.places_icons.add (icon);

      var glow = new Ctk.EffectGlow ();
      glow.set_color (white);
      glow.set_factor (1.0f);
      glow.set_margin (6);
      icon.add_effect (glow);

      this.pack (icon, false, false);
      icon.button_release_event.connect (this.on_button_release);

      if (this.places_icons.size == 1)
        {
          Clutter.Actor stage = icon.get_stage ();
          this.bg.create_places_background ((int)stage.width,
                                            (int)stage.height,
                                            (int)(this.padding.left + QL_PAD),
                                            (int)ICON_VIEW_WIDTH);
        }
    }

    public override void map ()
    {
      base.map ();
      this.bg.map ();
    }

    public override void unmap ()
    {
      base.unmap ();
      this.bg.unmap ();
    }

    public override void paint ()
    {
     this.bg.paint ();
     base.paint ();
    }

    public override void allocate (Clutter.ActorBox        box,
                                   Clutter.AllocationFlags flags)
    {
      Clutter.ActorBox        child_box = {0, 0, 0, 0};
      child_box.x1 = 0.0f;
      child_box.x2 = box.x2 - box.x1;
      child_box.y1 = 0.0f;
      child_box.y2 = box.y2 - box.y1;

      this.bg.allocate (child_box, flags);

      /* Allocate the places icons */
      var n_places = 0;
      var lpadding = this.padding.left + QL_PAD;

      child_box.y1 = ICON_VIEW_Y1;
      child_box.y2 = child_box.y1 + ICON_SIZE;

      foreach (PlaceIcon place in this.places_icons)
        {
          child_box.x1 = lpadding +  (ICON_VIEW_WIDTH * n_places);
          child_box.x2 = child_box.x1 +  ICON_VIEW_WIDTH;

          place.allocate (child_box, flags);

          n_places++;
        }

      /* Allocate the Trash */
      child_box.x1 = box.x2 - box.x1 - 266 - ICON_VIEW_WIDTH;
      child_box.x2 = child_box.x1 + ICON_VIEW_WIDTH;
      this.trash_icon.allocate (child_box, flags);

      /* Allocate the seperator */
      child_box.x1 -= 12.0f;
      child_box.x2 = child_box.x1 + 5;
      child_box.y1 = 10;
      child_box.y2 = ICON_SIZE;
      this.separator.allocate (child_box, flags);
    }

    public bool on_button_release (Clutter.Event event)
    {
      Clutter.Actor actor;
      actor = event.button.source;

      if (actor is PlaceIcon)
        {
          PlaceIcon     icon = actor as PlaceIcon;

          /* Do something with the click */
          if (actor == this.trash_icon)
            {
              try
                {
                  Process.spawn_command_line_async ("xdg-open trash:///");
                }
              catch (SpawnError e)
                {
                  warning ("Unable to show Trash: %s", e.message);
                }
            }
          else if (icon.place is Place)
            {
              Clutter.Actor stage = actor.get_stage ();

              /* Update the background */
              this.bg.create_places_background ((int)stage.width,
                                                (int)stage.height,
                                                (int)actor.x,
                                                80);

              /* Set the place as active, unset the others */
              foreach (PlaceIcon picon in this.places_icons)
                {
                  if (picon.place is Place)
                    picon.place.active = (picon == icon) ? true : false;
                }
            }

          return true;
        }

      return false;
    }
  }

  public class PlaceIcon : Ctk.Image
  {
    public Place? place { get; set; }

    public PlaceIcon (int       width,
                      string name,
                      string    icon_name,
                      string    tooltip)
    {
      Object (size:width);

      this.set_from_filename (icon_name);
      this.reactive = true;
    }

    public PlaceIcon.from_place (int size, Place place)
    {
      this(size, place.name, place.icon_name, "");
      this.place = place;
    }
  }

  public class PlacesBackground : Ctk.Bin
  {
    public Clutter.CairoTexture cairotxt;

    private Ctk.EffectGlow effect_glow;


    /* margin outside of the cairo texture. We draw outside to complete the
     * line loop and we don't want the line loop to be visible in some parts of
     * the screen.
     */
    private int margin = 5;

    /* (X, Y) origine of Places Bar: The top-left corner of the screen */
    private int place_x = 0;
    private int place_y = 0;

    /* Places Bar width and height. The width is set the the width of the
     * window.
     */
    private int place_w = 760;
    private int place_h = 55;

    private int place_bottom; /* place_y + place_h */

    /* Menu area width and height */
    private int menu_h = 22;
    private int menu_w = 216;

    private int rounding = 16;
    private int rounding_small = 8;

    private int menu_bottom; /* place_y + menu_h */

    private int tab_h = 50;

    /* The squirl area width */
    private int squirl_w = 70;

    public struct TabRect
    {
      int left;
      int right;
      int top;
      int bottom;
    }

    public int place_width;

    public PlacesBackground ()
    {
      place_width = 0;
    }

    construct
    {
      effect_glow = new Ctk.EffectGlow ();
      Clutter.Color c = Clutter.Color ()
      {
        red = 255,
        green = 255,
        blue = 255,
        alpha = 255
      };

      effect_glow.set_color (c);
      effect_glow.set_factor (1.0f);
      effect_glow.set_margin (5);
      this.add_effect (effect_glow);
    }

    private void draw_around_menu (Cairo.Context cairoctx)
    {
      cairoctx.line_to (place_x + place_w + margin, place_y + menu_h);
      cairoctx.line_to (place_x + place_w - menu_w + rounding,
                        place_y + menu_h);
      cairoctx.curve_to (place_x + place_w - menu_w, place_y + menu_h,
                         place_x + place_w - menu_w, place_y + menu_h,
                         place_x + place_w - menu_w, place_y+menu_h+rounding);
      cairoctx.line_to (place_x + place_w - menu_w,
                        place_y + place_h - rounding);
      cairoctx.curve_to (place_x + place_w - menu_w, place_y + place_h,
                         place_x + place_w - menu_w, place_y + place_h,
                         place_x + place_w - menu_w - rounding,
                         place_y + place_h);
    }

    private void draw_tab (Cairo.Context cairoctx, TabRect tab)
    {
      cairoctx.line_to (tab.right + rounding_small, place_bottom );
      cairoctx.curve_to (tab.right, place_bottom,
                         tab.right, place_bottom,
                         tab.right, place_bottom - rounding_small);
      cairoctx.line_to (tab.right, tab.top + rounding);
      cairoctx.curve_to (tab.right, tab.top,
                         tab.right, tab.top,
                         tab.right - rounding, tab.top);
      cairoctx.line_to (tab.left + rounding, tab.top);
      cairoctx.curve_to (tab.left, tab.top,
                         tab.left, tab.top,
                         tab.left, tab.top + rounding);
      cairoctx.line_to (tab.left, place_bottom - rounding_small);
      cairoctx.curve_to (tab.left, place_bottom,
                         tab.left, place_bottom,
                         tab.left - rounding_small, place_bottom);
    }

    private void draw_squirl(Cairo.Context cairoctx)
    {
      cairoctx.line_to (place_x + squirl_w + rounding_small, place_bottom);
      cairoctx.curve_to (place_x + squirl_w, place_bottom,
                         place_x + squirl_w, place_bottom,
                         place_x + squirl_w, place_bottom - rounding_small);
      cairoctx.line_to (place_x + squirl_w, menu_bottom + rounding );
      cairoctx.curve_to (place_x + squirl_w, menu_bottom,
                         place_x + squirl_w, menu_bottom,
                         place_x + squirl_w - rounding, menu_bottom);
      cairoctx.line_to (place_x - margin, menu_bottom);
    }

    public void create_places_background (int window_width,
                                          int window_height,
                                          int tab_position_x,
                                          int tab_width)
    {
      place_width = window_width;
      place_w = window_width;
      place_bottom = place_y + place_h;
      menu_bottom = place_y + menu_h;

      if (this.get_child () is Clutter.Actor)
        this.remove_actor (this.get_child ());

      cairotxt = new Clutter.CairoTexture(place_w, place_h + margin);
      Cairo.Context cairoctx = cairotxt.create();
      {
        cairoctx.set_source_rgba (1, 1, 1, 1.0);
        cairoctx.set_line_width (1.0);

        cairoctx.move_to (place_x - margin, place_y - margin);
        cairoctx.line_to (place_x + place_w + margin, place_y - margin);

        draw_around_menu (cairoctx);

        TabRect tab = TabRect();
        tab.left = tab_position_x;
        tab.right = tab.left + tab_width;
        tab.top = place_y + place_h - tab_h;
        tab.bottom = place_y + place_h;

        draw_tab(cairoctx, tab);

        draw_squirl (cairoctx);

        /* close the path */
        cairoctx.line_to (place_x - margin, place_y - margin);

        cairoctx.stroke_preserve ();
        cairoctx.set_source_rgba (1, 1, 1, 0.15);
        cairoctx.fill ();
      }

      cairotxt.set_opacity (0xFF);
      this.add_actor (cairotxt);
    }
  }

  public class PlacesVSeparator : Ctk.Bin
  {
    public int Width = 0;
    public int Height = 0;

    public Clutter.CairoTexture cairotxt;

    public PlacesVSeparator ()
    {
      this.create_separator (5, 48);
    }

    public void create_separator (int W, int H)
    {
      Width = W;
      Height = H;

      if (this.get_child () is Clutter.Actor)
        this.remove_actor (this.get_child ());

      cairotxt = new Clutter.CairoTexture(Width, Height);
      Cairo.Context cairoctx = cairotxt.create();
      {
        cairoctx.set_source_rgba (1, 1, 1, 1.0);
        cairoctx.set_line_width (1.0);

        cairoctx.move_to (Width/2.0, 0);
        cairoctx.line_to (Width/2.0, Height);

        cairoctx.stroke ();
        cairoctx.set_source_rgba (1, 1, 1, 0.15);
      }

      cairotxt.set_opacity (0xFF);
      this.add_actor (cairotxt);
    }

    construct
    {
      /*
      Ctk.EffectGlow effect_glow = new Ctk.EffectGlow ();
      Clutter.Color c = Clutter.Color ()
      {
        red = 255,
        green = 255,
        blue = 255,
        alpha = 255
      };

      effect_glow.set_color (c);
      effect_glow.set_factor (1.0f);
      effect_glow.set_margin (5);
      this.add_effect (effect_glow);
      */
    }
  }
}
