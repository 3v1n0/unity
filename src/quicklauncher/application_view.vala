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
 *
 */

namespace Unity.Quicklauncher
{  
  const string THROBBER_FILE = Unity.PKGDATADIR 
    + "/quicklauncher_spinner.png";
  const string FOCUSED_FILE  = Unity.PKGDATADIR 
    + "/quicklauncher_focused_indicator.svg";
  const string RUNNING_FILE  = Unity.PKGDATADIR 
    + "/quicklauncher_running_indicator.svg";
  const string HONEYCOMB_MASK_FILE = Unity.PKGDATADIR 
    + "/honeycomb-mask.png";

  public class ApplicationView : Ctk.Bin
  {
    
    public Launcher.Application app;
    private Ctk.Image icon;
    private Ctk.EffectGlow icon_glow_effect;
    private Ctk.EffectDropShadow icon_dropshadow_effect;
    private bool _is_sticky;
    private Clutter.Group container;
    private Ctk.Image throbber;
    private Ctk.Image focused_indicator;
    private Ctk.Image running_indicator;
    private Gdk.Pixbuf honeycomb_mask;
    public Unity.TooltipManager.Tooltip  tooltip;
    

    private bool _busy;
    private bool is_starting {
      get { return _busy; }
      set {
        if (value)
        {
          if (!_busy)
            throbber_start ();
        } else {
          throbber_hide ();
        }
        _busy = value;
      }
    }

    private Clutter.Animation anim_throbber;

    private void throbber_start ()
    {
      if (anim_throbber != null)
        anim_throbber.completed ();

      this.throbber.opacity = 255;
      this.throbber.set_z_rotation_from_gravity (0.0f, Clutter.Gravity.CENTER);
      this.anim_throbber = this.throbber.animate (
        Clutter.AnimationMode.LINEAR, 1200,
        "rotation-angle-z", 360.0f
        );
      this.anim_throbber.loop = true;

      GLib.Timeout.add_seconds (15, on_launch_timeout);
    }

    private void throbber_fadeout ()
    {
      if (this.throbber.opacity == 0)
        return;
      
      if (anim_throbber != null)
        anim_throbber.completed ();
      
      this.anim_throbber =
      this.throbber.animate (Clutter.AnimationMode.EASE_IN_QUAD,
                             200, 
                             "opacity", 
                             0);
      this.anim_throbber.loop = false;
    }
    
    private void throbber_hide ()
    {
      if (this.throbber.opacity > 0)
        throbber_fadeout ();
    }
    
    /* animation support */
    private Clutter.Animation _anim;
    public Clutter.Animation anim { 
      get { return _anim; }
      set {
        if (_anim != null) { 
          assert (_anim is Clutter.Animation);
          _anim.completed();
        }
        _anim = value;
      }
    }
    
    private Clutter.Animation hover_anim;
    
    /* if we are not sticky anymore and we are not running, request remove */
    public bool is_sticky {
      get { return _is_sticky; }
      set {
        if (value == false && !is_running) 
          this.request_remove (this);
        _is_sticky = value;
      }
    }
    
    public bool is_running {
      get { return app.running; }
    }
    
    public bool is_hovering = false;
    
    /**
     * signal is called when the application is not marked as sticky and 
     * it is not running
     */
    public signal void request_remove (ApplicationView app);
    public signal void request_attention (ApplicationView app);
    
    public ApplicationView (Launcher.Application app)
    {
      /* This is a 'view' for a launcher application object
       * it will (hopefully) be able to launch, track when this application 
       * opens and closes and be able to get desktop file info
       */
      this.app = app;
      generate_view_from_app ();
      this.app.opened.connect(this.on_app_opened);
      this.app.closed.connect(this.on_app_closed);

      this.tooltip = Unity.TooltipManager.get_default().create (this.app.name);

      this.app.notify["running"].connect (this.notify_on_is_running);
      this.app.notify["focused"].connect (this.notify_on_is_focused);
      notify_on_is_running ();
      notify_on_is_focused ();
    }

    construct 
    {
      this.container = new Clutter.Group ();
      add_actor (this.container);

      this.icon = new Ctk.Image (42);
      this.container.add_actor(this.icon);

      load_textures ();
        
      button_release_event.connect(this.on_pressed);

      /* hook up glow for enter/leave events */
      enter_event.connect(this.on_mouse_enter);
      leave_event.connect(this.on_mouse_leave);

      /* hook up tooltip for enter/leave events */
      this.icon.enter_event.connect (this.on_enter);
      this.icon.leave_event.connect (this.on_leave);
      this.icon.set_reactive (true);
      
      //icon_dropshadow_effect = new Ctk.EffectDropShadow(3, 1, 1);
      //this.icon.add_effect(icon_dropshadow_effect);
      this.icon.queue_relayout();
      
      set_reactive(true);
    }

    private void load_textures ()
    {      
      this.throbber = new Ctk.Image.from_filename (20, THROBBER_FILE);
      this.throbber.set_z_rotation_from_gravity (0.0f, Clutter.Gravity.CENTER);
      this.container.add_actor (this.throbber);
                                
      this.focused_indicator = new Ctk.Image.from_filename (8, FOCUSED_FILE);
      this.container.add_actor (this.focused_indicator);

      this.running_indicator = new Ctk.Image.from_filename (8, RUNNING_FILE);
      this.container.add_actor (this.running_indicator);
      
      this.throbber.set_opacity (0);
      this.focused_indicator.set_opacity (0);
      this.running_indicator.set_opacity (0);
      
      this.honeycomb_mask = new Gdk.Pixbuf.from_file(HONEYCOMB_MASK_FILE);
    
      relayout ();
    }

    /**
     * re-layouts the various indicators and throbbers in out view 
     */
    private void relayout ()
    {
      this.throbber.set_position (this.container.width - this.throbber.width,
                                  this.container.height - this.throbber.height);

      float mid_point_y = this.container.height / 2.0f;
      float focus_halfy = this.focused_indicator.height / 2.0f;
      float focus_halfx = this.container.width - this.focused_indicator.width;

      this.focused_indicator.set_position(focus_halfx + 12, 
                                          mid_point_y - focus_halfy);
      this.running_indicator.set_position (0, mid_point_y - focus_halfy);

      this.icon.set_position (6, 0);
    }
    
    /** 
     * taken from the prototype code and shamelessly stolen from
     * netbook launcher. needs to be improved at some point to deal
     * with all cases, it will miss some apps at the moment
     */
    static Gdk.Pixbuf make_icon(string? icon_name) 
    {
      /*
       * This code somehow manages to miss a lot of icon names 
       * (non found icons are replaced with stock missing image icons)
       * which is a little strange as I ported this code fron netbook launcher
       * pixbuf-cache.c i think, If anyone else has a better idea for this then
       * please give it a go. otherwise i will revisit this code the last week 
       * of the month sprint
       */
      Gdk.Pixbuf pixbuf;
      Gtk.IconTheme theme = Gtk.IconTheme.get_default ();
      
      if (icon_name == null)
        {
          pixbuf = theme.load_icon(Gtk.STOCK_MISSING_IMAGE, 42, 0);
          return pixbuf;
        }
        
      if (icon_name.has_prefix("file://")) 
        {
          string filename = "";
          /* this try/catch sort of isn't needed... but it makes valac stop 
           * printing warning messages
           */
          try 
          {
            filename = Filename.from_uri(icon_name);
          } 
          catch (GLib.ConvertError e)
          {
          }
          if (filename != "") 
            {
              pixbuf = new Gdk.Pixbuf.from_file_at_scale(icon_name, 
                                                         42, 42, true);
              if (pixbuf is Gdk.Pixbuf)
                  return pixbuf;
            }
        }
      
      if (Path.is_absolute(icon_name))
        {
          if (FileUtils.test(icon_name, FileTest.EXISTS)) 
            {
              pixbuf = new Gdk.Pixbuf.from_file_at_scale(icon_name, 
                                                         42, 42, true);

              if (pixbuf is Gdk.Pixbuf)
                return pixbuf;
            }
        }

      if (FileUtils.test ("/usr/share/pixmaps/" + icon_name, 
                          FileTest.IS_REGULAR))
        {
          pixbuf = new Gdk.Pixbuf.from_file_at_scale (
            "/usr/share/pixmaps/" + icon_name, 42, 42, true
            );
          
          if (pixbuf is Gdk.Pixbuf)
            return pixbuf;
        }
      
      Gtk.IconInfo info = theme.lookup_icon(icon_name, 42, 0);
      if (info != null) 
        {
          string filename = info.get_filename();
          if (FileUtils.test(filename, FileTest.EXISTS)) 
            {
              pixbuf = new Gdk.Pixbuf.from_file_at_scale(filename, 
                                                         42, 42, true);
            
              if (pixbuf is Gdk.Pixbuf)
                return pixbuf;
            }
        }
      
      try 
      {
        pixbuf = theme.load_icon(icon_name, 42, Gtk.IconLookupFlags.FORCE_SVG);
      }
      catch (GLib.Error e) 
      {
        warning ("could not load icon for %s - %s", icon_name, e.message);
        pixbuf = theme.load_icon(Gtk.STOCK_MISSING_IMAGE, 42, 0);
        return pixbuf;
      }
      return pixbuf;
          
    }
    
    /**
     * goes though our launcher application and generates icons and such
     */
    private void generate_view_from_app ()
    {
      var pixbuf = make_icon (app.icon_name);
      this.icon.set_from_pixbuf (pixbuf);
    }
  
    private void on_app_opened (Wnck.Application app) 
    {
      this.is_starting = false;
    }

    private void on_app_closed (Wnck.Application app) 
    {
      this.is_starting = false;
      if (!this.is_running && !this.is_sticky) 
        this.request_remove (this);
    }

    private bool on_enter (Clutter.Event event)
    {
      float xx, xy;

      get_transformed_position (out xx, out xy);
      xx += width + 4;
      /* TODO: 30 is the default tooltip height */
      xy += (height - 30) / 2;
      Unity.TooltipManager.get_default().show (this.tooltip, (int) xx, (int) xy);

      return false;
    } 

    private bool on_leave (Clutter.Event event)
    {
      Unity.TooltipManager.get_default().hide (this.tooltip);
      return false;
    }

    private bool on_pressed(Clutter.Event src) 
    {
      /* hide the tooltip, to prevent it from stealing focus when
	 the app starts and is focused */
      Unity.TooltipManager.get_default().hide (this.tooltip);

      if (is_starting)
	return true;

      if (app.running)
      {
        // we only want to switch to the application, not launch it
        app.show ();
      }
      else 
      {
	this.is_starting = true;
        try 
        {
          app.launch ();
        } 
        catch (GLib.Error e)
        {
          critical ("could not launch application %s: %s", 
                    this.name, 
                    e.message);
	  this.is_starting = false;
        }
      }
      
      return true;
    }
    
    private bool on_mouse_enter(Clutter.Event src) 
    {
      this.is_hovering = true;
      icon_glow_effect = new Ctk.EffectGlow();
      Clutter.Color c = Clutter.Color() {
        red = 255,
        green = 255,
        blue = 255,
        alpha = 255
      };
      icon_glow_effect.set_background_texture(honeycomb_mask);
      icon_glow_effect.set_color(c);
      icon_glow_effect.set_factor(1.0f);
      this.icon.add_effect(icon_glow_effect);
      this.icon.queue_relayout();

      this.hover_anim = icon_glow_effect.animate (
        Clutter.AnimationMode.EASE_IN_OUT_SINE, 600, "factor", 8.0f);
      this.hover_anim.completed.connect (on_hover_anim_completed);

      return false;
    }
    
    private bool on_launch_timeout ()
    {
      this.is_starting = false;

      return false;
    }

    private void on_hover_anim_completed ()
    {
      if (is_hovering)
      {
        Idle.add (do_new_anim);
      }
    }

    private bool do_new_anim ()
    {
      float fadeto = 1.0f;
      if (icon_glow_effect.get_factor() <= 1.1f)
        fadeto = 8.0f;

      this.hover_anim = icon_glow_effect.animate(
        Clutter.AnimationMode.EASE_IN_OUT_CIRC, 600, "factor", fadeto);
      this.hover_anim.completed.connect (on_hover_anim_completed);
      return false;
    }
    
    
    private bool on_mouse_leave(Clutter.Event src) 
    {
      this.is_hovering = false;
      this.hover_anim.completed ();
      this.icon.remove_effect(this.icon_glow_effect);
      this.icon.queue_relayout();

      return false;
    }
    
    /**
     * if the application is not running anymore and we are not sticky, we
     * request to be removed
     */
    private void notify_on_is_running ()
    {
      this.is_starting = false;

      /* we need to show the running indicator when we are running */
      if (this.is_running)
      {
        this.running_indicator.set_opacity (255);
      }
      else
      {
        this.running_indicator.set_opacity (0);
        this.focused_indicator.set_opacity (0);
      }       

      if (!this.is_running && !this.is_sticky)
          this.request_remove (this);
    }
   
    private void notify_on_is_focused ()
    {
      if (app.focused) {
        this.focused_indicator.set_opacity (255);
        this.request_attention (this);
      }
      else
        this.focused_indicator.set_opacity (0);

      this.is_starting = false;
    }

  }

}
