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
using Unity.Quicklauncher.Models;

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
  
  const uint SHORT_DELAY = 400;
  const uint MEDIUM_DELAY = 800;
  const uint LONG_DELAY = 1600;


  public class LauncherView : Ctk.Bin
  {

    public LauncherModel? model;

    /* the prettys */
    private Ctk.Image icon;
    private Ctk.Image throbber;
    private Ctk.Image focused_indicator;
    private Ctk.Image running_indicator;
    private Gdk.Pixbuf honeycomb_mask;
    private Clutter.Group container;

    private Ctk.Menu menu;
    private Ctk.EffectDropShadow menu_dropshadow;
    private Gee.List<ShortcutItem> offline_shortcuts;

    private Ctk.EffectGlow effect_icon_glow;
    private Ctk.EffectDropShadow effect_icon_dropshadow;
    
    /* internal view logic datatypes */
    private uint32 last_pressed_time;

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
    
    public bool is_hovering = false;

    /**
     * signal is called when the application is not marked as sticky and 
     * it is not running
     */
    public signal void request_remove ();
    public signal void request_attention ();
    public signal void clicked ();


    /* animations */
    
    private Clutter.Animation anim_throbber;
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

  /* constructors */
    public LauncherView (LauncherModel model)
      {
        /* this is a "view" for a launchermodel object */
        this.model = model;
        this.model.notify_active.connect (this.notify_on_is_running);
        this.model.notify_focused.connect (this.notify_on_is_focused);

        notify_on_is_running ();
        notify_on_is_focused ();

        /* get the graphic from the model */
        this.notify_on_icon ();
        this.model.notify["icon"].connect (this.notify_on_icon);
      }

    construct 
      {
        /* !!FIXME!! - we need to allocate ourselfs instead of using the
         * clutter group, or have a Ctk.Group, it makes sure that the 
         * bubbling out systems in ctk work
         */
        this.container = new Clutter.Group ();
        add_actor (this.container);

        this.icon = new Ctk.Image (46);
        this.container.add_actor (this.icon);

        load_textures ();

        button_press_event.connect (this.on_pressed);
        button_release_event.connect (this.on_released);
        
        enter_event.connect (this.on_mouse_enter);
        leave_event.connect (this.on_mouse_leave);
        
        this.clicked.connect (this.on_clicked);
        this.icon.do_queue_redraw ();

        effect_icon_dropshadow = new Ctk.EffectDropShadow(3, 5, 5);
        this.icon.add_effect(effect_icon_dropshadow);
      
        set_reactive (true);
      }

    private void load_textures ()
    {      
      this.throbber = new Ctk.Image.from_filename (20, THROBBER_FILE);
      this.throbber.set_z_rotation_from_gravity (0.0f, 
                                                 Clutter.Gravity.CENTER);
      this.container.add_actor (this.throbber);
                                
      this.focused_indicator = new Ctk.Image.from_filename (8, 
                                                            FOCUSED_FILE);
      this.container.add_actor (this.focused_indicator);

      this.running_indicator = new Ctk.Image.from_filename (8, 
                                                            RUNNING_FILE);
      this.container.add_actor (this.running_indicator);
      
      this.throbber.set_opacity (0);
      this.focused_indicator.set_opacity (0);
      this.running_indicator.set_opacity (0);
      
      try
        {
          this.honeycomb_mask = new Gdk.Pixbuf.from_file(HONEYCOMB_MASK_FILE);
        }
      catch (Error e)
        {
          warning ("Unable to load asset %s: %s",
                   HONEYCOMB_MASK_FILE,
                   e.message);
        }
    
      relayout ();
    }

    /**
     * re-layouts the various indicators and throbbers in out view 
     */
    private void relayout ()
    {
      this.throbber.set_position (container.width - this.throbber.width,
                                  container.height - this.throbber.height);

      float mid_point_y = this.container.height / 2.0f;
      float focus_halfy = this.focused_indicator.height / 2.0f;
      float focus_halfx = container.width - this.focused_indicator.width;

      this.focused_indicator.set_position(focus_halfx + 12, 
                                          mid_point_y - focus_halfy);
      this.running_indicator.set_position (0, mid_point_y - focus_halfy);

      this.icon.set_position (6, 0);
    }

    /* animation logic */
    private void throbber_start ()
    {
      if (anim_throbber != null)
        anim_throbber.completed ();

      this.throbber.opacity = 255;
      this.throbber.set_z_rotation_from_gravity (0.0f, 
                                                 Clutter.Gravity.CENTER);
      this.anim_throbber = this.throbber.animate (
        Clutter.AnimationMode.LINEAR, LONG_DELAY,
        "rotation-angle-z", 360.0f
        );

      this.anim_throbber.loop = true;
      GLib.Timeout.add_seconds (8, on_launch_timeout);
    }
    
    private void throbber_fadeout ()
    {
      if (this.throbber.opacity == 0)
        return;
      
      if (anim_throbber != null)
        anim_throbber.completed ();
      
      this.anim_throbber =
      this.throbber.animate (Clutter.AnimationMode.EASE_IN_QUAD,
                             SHORT_DELAY, 
                             "opacity", 
                             0);
      this.anim_throbber.loop = false;
    }
    
    private void throbber_hide ()
    {
      if (this.throbber.opacity > 0)
        throbber_fadeout ();
    }


    private void start_hover_anim ()
    {
      this.hover_anim = effect_icon_glow.animate (
        Clutter.AnimationMode.EASE_IN_OUT_SINE, SHORT_DELAY, 
        "factor", 1.0f);
      this.hover_anim.completed.connect (on_hover_anim_completed);
    }

    private void on_hover_anim_completed ()
    {
      if (is_hovering)
      {
        Idle.add (do_new_hover_anim);
      }
    }

    private bool do_new_hover_anim ()
    {
      float fadeto = 0.0f;
      if (effect_icon_glow.get_factor() <= 0.0f)
        fadeto = 1.0f;
      
      this.hover_anim = effect_icon_glow.animate(
        Clutter.AnimationMode.EASE_IN_OUT_CIRC, 600, "factor", fadeto);
      this.hover_anim.completed.connect (on_hover_anim_completed);
      return false;
    }

    /* callbacks on model */
    private void notify_on_icon ()
      {
        if (this.model.icon is Gdk.Pixbuf)
          {
            this.icon.set_from_pixbuf (this.model.icon);
          }
        else 
          {
            this.icon.set_from_stock (Gtk.STOCK_MISSING_IMAGE);
          }
      }

    /*
     * if the application is not running anymore and we are not sticky, we
     * request to be removed
     */
    private void notify_on_is_running ()
    {
      this.is_starting = false;
      
      /* we need to show the running indicator when we are running */
      if (this.model.is_active)
        {
          this.running_indicator.set_opacity (255);
        }
      else
        {
          this.running_indicator.set_opacity (0);
          this.focused_indicator.set_opacity (0);
        }        
      
      if (!this.model.is_active && !this.model.is_sticky)
        {
          this.request_remove ();
        }
    }

    private void notify_on_is_focused ()
    {
      if (this.model.is_focused) 
        {
          this.focused_indicator.set_opacity (255);
        }
      else
        {
          this.focused_indicator.set_opacity (0);
        }

      this.is_starting = false;
    }


    /* callbacks on self */

    private bool on_mouse_enter (Clutter.Event event)
    {
      this.is_hovering = true;
      effect_icon_glow = new Ctk.EffectGlow ();
      Clutter.Color c = Clutter.Color () {
        red = 255,
        green = 255,
        blue = 255,
        alpha = 255
      };
      effect_icon_glow.set_background_texture (honeycomb_mask);
      effect_icon_glow.set_color (c);
      effect_icon_glow.set_factor (1.0f);
      this.icon.add_effect (effect_icon_glow);
      this.icon.do_queue_redraw ();

      start_hover_anim ();

      return false;
    } 

    private bool on_launch_timeout ()
    {
      this.is_starting = false;

      return false;
    }

    private bool on_mouse_leave(Clutter.Event src) 
    {
      if (this.is_hovering)
        {
          this.is_hovering = false;
          this.hover_anim.completed ();
          this.icon.remove_effect(this.effect_icon_glow);
          this.icon.queue_relayout();
        }

      return false;
    }

    private bool on_pressed(Clutter.Event src) 
    {
      var bevent = src.button;
      if (bevent.button == 1)
      {
        last_pressed_time = bevent.time;
      } 
      else 
      {
        build_quicklist ();
      }
      return false;
    }
    
    private void build_quicklist ()
    {
      var items = this.model.get_menu_shortcuts ();
      this.offline_shortcuts = items;
      this.menu = new Ctk.Menu ();
      Clutter.Stage stage = this.get_stage() as Clutter.Stage;
      stage.add_actor (this.menu);
      
      float x, y;
      this.get_transformed_position (out x, out y);
      this.menu.attach_to_actor (this);
      
      Ctk.Padding padding = Ctk.Padding () {
        left = 6, 
        right = 6,
        top = 6,
        bottom = 6
      };
      this.menu.set_padding (padding);
      
      foreach (ShortcutItem shortcut in offline_shortcuts)
      {
        Ctk.MenuItem menuitem = new Ctk.MenuItem.with_label (
                                                    shortcut.get_name ()); 
        this.menu.append (menuitem);
        menuitem.activated.connect (shortcut.activated);
        menuitem.activated.connect (this.close_menu);
      }
      menu_dropshadow = new Ctk.EffectDropShadow(3, 5, 5);
      this.icon.add_effect(menu_dropshadow);
      this.menu.show ();
      
    }
    
    private void close_menu ()
    {
      this.menu.remove_effect(this.menu_dropshadow);
      Clutter.Stage stage = this.get_stage() as Clutter.Stage;
      stage.remove_actor (this.menu);
    }

    private bool on_released (Clutter.Event src)
    {
      var bevent = src.button;
      if (bevent.button == 1 &&
          (bevent.time - last_pressed_time) < 500)
      {
        this.clicked ();
      }
      return false;
    }

    private void on_clicked ()
    {      
      if (is_starting)
      {
        return;
      }
      this.model.activate ();
    }
  }
}



