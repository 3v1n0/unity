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
 *
 */

namespace Unity.Places.Bar
{
  const string TRASH_FILE = Unity.PKGDATADIR + "/trash.png";

  public class View : Ctk.Box
  {
    public Places.Model model { get; construct; }

    public int IconSize = 0;
    public int FirstPlaceIconPosition = 0;
    public int PlaceIconSpacing = 0;
    public int SeparatorPosition = 0;

    public int FirstDevicePosition = 0;
    public int DeviceIconSpacing = 0;

    public int TrashPosition = 0;

    private Gee.ArrayList<PlaceIcon> PlacesIconArray;
    private Gee.ArrayList<PlaceIcon> DevicesIconArray;
    private PlaceIcon                trash_icon;

    private PlacesVSeparator Separator;
    private PlacesBackground bg;

    public signal void sig_places_active_icon_index (int i);
    public signal void sig_devices_active_icon_index (int i);
    public signal void sig_trash_active_icon_index ();

    public View (Places.Model model)
    {
      int                    i;
      int                    icon_size = 48;
      Ctk.EffectGlow         glow;
      Clutter.Color          white = {255, 255, 255, 255};

      Object (model:model);
      this.model.place_added.connect (this.on_place_added);

      this.homogeneous  = false;
      this.orientation  = Ctk.Orientation.HORIZONTAL;
      this.spacing = 18;

      this.bg = new PlacesBackground ();
      this.bg.set_parent (this);
      this.bg.show ();

      this.PlacesIconArray = new Gee.ArrayList<PlaceIcon> ();
      this.DevicesIconArray = new Gee.ArrayList<PlaceIcon> ();

      /* create all image-actors for icons */
      for (i = 0; i < this.DevicesIconArray.size ; i++)
      {
        glow = new Ctk.EffectGlow ();
        glow.set_color (white);
        glow.set_factor (1.0f);
        glow.set_margin (6);
        this.DevicesIconArray[i].add_effect (glow);

        this.pack (this.DevicesIconArray[i], false, false);
        this.DevicesIconArray[i].enter_event.connect (this.on_enter);
        this.DevicesIconArray[i].leave_event.connect (this.on_leave);
        this.DevicesIconArray[i].button_press_event.connect (this.on_button_press);
      }

      {
        trash_icon = new PlaceIcon (icon_size,
                                   "Trash",
                                   TRASH_FILE,
                                   "Your piece of waste");
        glow = new Ctk.EffectGlow ();
        glow.set_color (white);
        glow.set_factor (1.0f);
        glow.set_margin (6);
        this.trash_icon.add_effect (glow);

        this.pack (this.trash_icon, false, false);
        this.trash_icon.enter_event.connect (this.on_enter);
        this.trash_icon.leave_event.connect (this.on_leave);
        this.trash_icon.button_press_event.connect (this.on_button_press);
      }

      Separator = new PlacesVSeparator ();
      this.pack (this.Separator, false, false);

      this.show_all ();
    }

    private void on_place_added (Place place)
    {
      int           icon_size = 48;
      Clutter.Color white = {255, 255, 255, 255};

      var icon = new PlaceIcon (icon_size,
                                place.name,
                                place.icon_name,
                                "");
      this.PlacesIconArray.add (icon);

      var glow = new Ctk.EffectGlow ();
      glow.set_color (white);
      glow.set_factor (1.0f);
      glow.set_margin (6);
      icon.add_effect (glow);

      this.pack (icon, false, false);
      icon.enter_event.connect (this.on_enter);
      icon.leave_event.connect (this.on_leave);
      icon.button_press_event.connect (this.on_button_press);

      if (this.PlacesIconArray.size == 1)
        {
          Clutter.Actor stage = icon.get_stage ();
          this.bg.create_places_background ((int)stage.width,
                                            (int)stage.height,
                                            (int)this.padding.left + 12,
                                            80);
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
      child_box.x1 = this.padding.left + 24;
      child_box.y1 = 8;
      child_box.y2 = child_box.y1 + 48;
      var n_places = 0;
      foreach (PlaceIcon place in this.PlacesIconArray)
        {
          child_box.x1 = this.padding.left + 12.0f + (80.0f * n_places);
          child_box.x2 = child_box.x1 +  80.0f;

          place.allocate (child_box, flags);

          n_places++;
        }

      /* Allocate the Trash */
      child_box.x1 = box.x2 - box.x1 - 266 - 80.0f;
      child_box.x2 = child_box.x1 + 80.0f;
      this.trash_icon.allocate (child_box, flags);

      /* Allocate the seperator */
      child_box.x1 -= 12.0f;
      child_box.x2 = child_box.x1 + 5;
      child_box.y1 = 10;
      child_box.y2 = 48;
      this.Separator.allocate (child_box, flags);
    }

    public int get_number_of_places ()
    {
      return this.PlacesIconArray.size;
    }

    public int get_number_of_devices ()
    {
      return this.DevicesIconArray.size;
    }

    public bool on_enter ()
    {
      /* stdout.printf ("on_enter() called\n"); */
      return false;
    }

    public bool on_leave ()
    {
      /* stdout.printf ("on_leave() called\n"); */
      return false;
    }

    public bool on_button_press (Clutter.Event event)
    {
      Clutter.Actor actor;
      actor = event.button.source;

      if (actor is PlaceIcon)
        {
          Clutter.Actor stage = actor.get_stage ();
          this.bg.create_places_background ((int)stage.width,
                                            (int)stage.height,
                                            (int)actor.x,
                                            80);
          return true;
        }

      return false;
    }

    construct
    {
    }
  }

  /* margin outside of the cairo texture. We draw outside to complete the
   * line loop and we don't want the line loop to be visible in some parts of
   * the screen.
   */
  private int margin = 5;

  public class PlacesBackground : Ctk.Bin
  {
    public Clutter.CairoTexture cairotxt;

    private Ctk.EffectGlow effect_glow;

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
      this.CreateSeparator (5, 48);
    }

    public void CreateSeparator (int W, int H)
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


  public class PlaceIcon : Ctk.Image
  {
    public Ctk.Image view;

    public PlaceIcon (int       width,
                      string name,
                      string    icon_name,
                      string    tooltip)
    {
      Object (size:width);

      this.set_from_filename (icon_name);
      this.reactive = true;
    }
  }
}
