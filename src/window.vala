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
 *             Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */

namespace Unity
{
  public class PlacesBackground : Ctk.Bin
  {
    public Clutter.CairoTexture cairotxt;

    private Ctk.EffectGlow effect_glow;

    /* (X, Y) origine of Places Bar: The top-left corner of the screen */
    private int PlaceX = 0;   
    private int PlaceY = 0;

    /* Places Bar width and height. The width is set the the width of the window. */
    private int PlaceW = 760;
    private int PlaceH = 55;

    private int PlaceBottom; //PlaceY + PlaceH;

    /* Margin outside of the cairo texture. We draw outside to complete the line loop
     * and we don't want the line loop to be visible in some parts of the screen. 
     * */
    private int Margin = 20;

    /* Menu area width and height */
    private int MenuH = 22;
    private int MenuW = 216;


    private int Rounding = 12;
    private int RoundingSmall = 8;

    private int MenuBottom; //PlaceY + MenuH;

    private int TabX = 170;
    private int TabW = 70;
    private int TabH = 50;

    /* The squirl area width */
    private int SquirlW = 70;

    public struct TabRect
	  {
	     int left;
	     int right;
	     int top;
	     int bottom;
	  }
	
  	void drawAroundMenu (Cairo.Context cairoctx)
  	{
	    cairoctx.line_to (PlaceX + PlaceW + Margin, PlaceY + MenuH);
  	  cairoctx.line_to (PlaceX + PlaceW - MenuW + Rounding, PlaceY + MenuH);
	    cairoctx.curve_to ( 
    		PlaceX + PlaceW - MenuW, PlaceY + MenuH,
    		PlaceX + PlaceW - MenuW, PlaceY + MenuH,
	    	PlaceX + PlaceW - MenuW, PlaceY + MenuH + Rounding);
  	  cairoctx.line_to (PlaceX + PlaceW - MenuW, PlaceY + PlaceH - Rounding); 
	    cairoctx.curve_to (
        PlaceX + PlaceW - MenuW, PlaceY + PlaceH,
    		PlaceX + PlaceW - MenuW, PlaceY + PlaceH,
    		PlaceX + PlaceW - MenuW - Rounding, PlaceY + PlaceH);
  	}

	  void drawTab(Cairo.Context cairoctx, TabRect tab)
  	{
	    cairoctx.line_to (tab.right + RoundingSmall, PlaceBottom );
  	  cairoctx.curve_to ( 
	    	tab.right, PlaceBottom,
  	  	tab.right, PlaceBottom,
    		tab.right, PlaceBottom - RoundingSmall);
  	  cairoctx.line_to (tab.right, tab.top + Rounding);
  	  cairoctx.curve_to (
    		tab.right, tab.top,
    		tab.right, tab.top,
    		tab.right - Rounding, tab.top);
  	  cairoctx.line_to (tab.left + Rounding, tab.top);
	    cairoctx.curve_to (
    		tab.left, tab.top,
    		tab.left, tab.top,
    		tab.left, tab.top + Rounding);
  	  cairoctx.line_to (tab.left, PlaceBottom - RoundingSmall);
  	  cairoctx.curve_to (
        tab.left, PlaceBottom,
    		tab.left, PlaceBottom,
    		tab.left - RoundingSmall, PlaceBottom);
  	}

	  void drawSquirl(Cairo.Context cairoctx)
  	{
  	  cairoctx.line_to (PlaceX + SquirlW + RoundingSmall, PlaceBottom);
  	  cairoctx.curve_to (
    		PlaceX + SquirlW, PlaceBottom,
    		PlaceX + SquirlW, PlaceBottom,
    		PlaceX + SquirlW, PlaceBottom - RoundingSmall);
  	  cairoctx.line_to (PlaceX + SquirlW, MenuBottom + Rounding );
  	  cairoctx.curve_to (
    		PlaceX + SquirlW, MenuBottom,
    		PlaceX + SquirlW, MenuBottom,
    		PlaceX + SquirlW - Rounding, MenuBottom);
  	  cairoctx.line_to (PlaceX - Margin, MenuBottom);
	  }
	
    public PlacesBackground (int WindowWidth, int WindowHeight)
    {
      PlaceW = WindowWidth;
      PlaceBottom = PlaceY + PlaceH;
      MenuBottom = PlaceY + MenuH;

      cairotxt = new Clutter.CairoTexture(PlaceW, PlaceH);
      Cairo.Context cairoctx = cairotxt.create();
      {
    		cairoctx.set_source_rgba (1, 1, 1, 1.0);
		    cairoctx.set_line_width (1.0);

    		cairoctx.move_to (PlaceX - Margin, PlaceY - Margin);
    		cairoctx.line_to (PlaceX + PlaceW + Margin, PlaceY - Margin);

		    drawAroundMenu (cairoctx);

    		TabRect tab = TabRect();
    		tab.left = TabX;
    		tab.right = tab.left + TabW;
    		tab.top = PlaceY + PlaceH - TabH;
    		tab.bottom = PlaceY + PlaceH;

		    drawTab(cairoctx, tab);

    		drawSquirl (cairoctx);

    		/* close the path */
		    cairoctx.line_to (PlaceX - Margin, PlaceY - Margin);

    		cairoctx.stroke_preserve ();
		    cairoctx.set_source_rgba (1, 1, 1, 0.15);
    		cairoctx.fill ();
      }

      cairotxt.set_opacity (0xFF);
      this.add_actor (cairotxt);

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
      this.add_effect (effect_glow);
    }

    construct
    {
    }
  }

  public class UnderlayWindow : Gtk.Window, Shell
  {
    public bool is_popup { get; construct; }
    public int  popup_width { get; construct; }
    public int  popup_height { get; construct; }
    
    private Wnck.Screen wnck_screen;
    private Workarea    workarea_size;

    private GtkClutter.Embed gtk_clutter;
    private Clutter.Stage    stage;
    private bool             is_showing;

    private Background          background;
    private Quicklauncher.View  quicklauncher;
    private Unity.Places.View   places;

    private PlacesBackground  placesbackground;
    /* These parameters are temporary until we get the right metrics for the places bar */
    private int               PlacesDecorationOffsetX;
    private int               PlacesDecorationOffsetY;

    public UnderlayWindow (bool popup, int width, int height)
    {
      Object(is_popup: popup, popup_width: width, popup_height: height);
    }
    
    construct
    {
      START_FUNCTION ();
      this.workarea_size = new Workarea ();
      this.workarea_size.update_net_workarea ();
      
      if (this.is_popup)
        {
          this.type_hint = Gdk.WindowTypeHint.NORMAL;
          this.decorated = true;
          this.skip_taskbar_hint = false;
          this.skip_pager_hint = false;
          this.delete_event.connect (() => 
            {
              Gtk.main_quit ();
              return false;
            }
          );
        }
      else
        {
          this.type_hint = Gdk.WindowTypeHint.DESKTOP;
          this.set_keep_below (true);
          this.decorated = false;
          this.skip_taskbar_hint = true;
          this.skip_pager_hint = true;
          this.accept_focus = false;
          this.can_focus = false;
          this.delete_event.connect (() => { return true; });
          this.screen.size_changed.connect ((s) => 
                                            { this.relayout (); });
          this.screen.monitors_changed.connect ((s) =>
                                                { this.relayout (); });
        }
      this.title = "Unity";
      this.icon_name = "distributor-logo";
      this.is_showing = false;
      
      /* Gtk.ClutterEmbed */
      this.realize ();

      this.gtk_clutter = new GtkClutter.Embed ();
      this.add (this.gtk_clutter);
      this.gtk_clutter.realize ();
      
      //setup dnd
      Gtk.TargetEntry[] target_list = {
        Gtk.TargetEntry () {target="STRING", flags=0, info=Unity.dnd_targets.TARGET_STRING },
        Gtk.TargetEntry () {target="text/plain", flags=0, info=Unity.dnd_targets.TARGET_STRING },
        Gtk.TargetEntry () {target="text/uri-list", flags=0, info=Unity.dnd_targets.TARGET_URL },
        Gtk.TargetEntry () {target="x-url/http", flags=0, info=Unity.dnd_targets.TARGET_URL },
        Gtk.TargetEntry () {target="x-url/ftp", flags=0, info=Unity.dnd_targets.TARGET_URL },
        Gtk.TargetEntry () {target="_NETSCAPE_URL", flags=0, info=Unity.dnd_targets.TARGET_URL }
      };

      Ctk.dnd_init (this.gtk_clutter, target_list);
      
      this.stage = (Clutter.Stage)this.gtk_clutter.get_stage ();
      
      Clutter.Color stage_bg = Clutter.Color () { 
          red = 0x00,
          green = 0x00,
          blue = 0x00,
          alpha = 0xff 
        };
      this.stage.set_color (stage_bg);
      this.stage.button_press_event.connect (this.on_stage_button_press);

      /* Components */
      this.background = new Background ();
      this.stage.add_actor (this.background);
      this.background.show ();

      this.quicklauncher = new Quicklauncher.View (this);
      this.places = new Unity.Places.View ();
      this.stage.add_actor (this.quicklauncher);
      this.stage.add_actor (this.places);
     
      /* Places Background */
      PlacesDecorationOffsetX = 6;
      PlacesDecorationOffsetY = 6;

      Gdk.Rectangle size;
      this.screen.get_monitor_geometry (0, out size);
      this.placesbackground = new PlacesBackground (size.width, size.height);
      this.stage.add_actor (this.placesbackground);
      this.placesbackground.show ();

      /* Layout everything */
      this.move (0, 0);
      this.relayout ();

      /* Window management */
      this.wnck_screen = Wnck.Screen.get_default ();
      if (!this.is_popup)
        {
          this.wnck_screen.active_window_changed.connect (this.on_active_window_changed);
        }

      /* inform TooltipManager about window */
      Unity.TooltipManager.get_default().top_level = this;
      END_FUNCTION ();
    }

    private void relayout ()
    {
      int x, y, width, height;

      if (this.is_popup)
        {
          x = 0;
          y = 0;
          width = this.popup_width;
          height = this.popup_height;
        }
      else
        {
          Gdk.Rectangle size;

          this.screen.get_monitor_geometry (0, out size);
          x = size.x;
          y = size.y;
          width = size.width;
          height = size.height;
        }

      debug ("relayout: %dx%d - %dx%d", x, y, width, height);

      this.resize (width, height);
      this.stage.set_size (width, height);

      if (!this.is_popup)
        Utils.set_strut ((Gtk.Window)this, 54, 0, height);

      /* Update component layouts */
      this.background.set_position (0, 0);
      this.background.set_size (width, height);
      
      this.quicklauncher.set_size (58, height);
      this.quicklauncher.set_position (this.workarea_size.left,
                                       this.workarea_size.top);

       this.places.set_size (width -
                             this.workarea_size.left -
                             this.workarea_size.right -
                             54,
                             height -
                             this.workarea_size.top -
                             this.workarea_size.bottom);
       this.places.set_position (this.workarea_size.left + 54 + PlacesDecorationOffsetX,
                                 this.workarea_size.top + PlacesDecorationOffsetY);

       this.placesbackground.set_position (this.workarea_size.left + 58, this.workarea_size.top);
    }

    public override void show ()
    {
      base.show ();
      this.gtk_clutter.show ();
      this.stage.show_all ();
    }

    /*
     * UNDERLAY WINDOW MANAGEMENT
     */
    public void on_active_window_changed (Wnck.Window? previous_window)
    {
      Wnck.Window new_window = this.wnck_screen.get_active_window ();
      if (new_window == null)
        return;
    
      /* FIXME: We want the second check to be a class_name or pid check */
      if (new_window is Wnck.Window 
          && new_window.get_type () != Wnck.WindowType.DESKTOP
          && new_window.get_name () == "Unity")
        {
          //this.wnck_screen.toggle_showing_desktop (true);
          this.is_showing = true;
        }
      else
        {
          /* FIXME: We want to suppress events getting to the stage at this
           * point, to stop the user accidently activating a control when they
           * just want to switch to the launcher
           */
          this.is_showing = false;
        }
    }

    public bool on_stage_button_press (Clutter.Event src)
    {
      if (this.is_popup)
        return false;

      if (this.is_showing)
        {
          ;
        }
      else
        {
          //this.wnck_screen.toggle_showing_desktop (true);
        }
      return false;
    }

    /*
     * SHELL IMPLEMENTATION
     */
    public Clutter.Stage get_stage ()
    {
      return this.stage;
    }

    public ShellMode get_mode ()
    {
      return ShellMode.UNDERLAY;
    }

    public void show_unity ()
    {
      this.wnck_screen.toggle_showing_desktop (true);
    }

  }
 
  public class Workarea 
  {
    public signal void workarea_changed ();
    public int left;
    public int top;
    public int right;
    public int bottom;
    
    public Workarea ()
    {
      left = 0;
      right = 0;
      top = 0;
      bottom = 0;
      
      update_net_workarea ();
    }

    public void update_net_workarea () 
    {
      /* FIXME - steal code from the old liblauncher to do this
       * (launcher-session.c) this is just fake code to get it running
       */
      left = 0;
      right = 0;
      top = 24;
      bottom = 0;
    }
  }  
}
