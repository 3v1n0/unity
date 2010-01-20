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
  const string FOCUSED_FILE  = Unity.PKGDATADIR
    + "/quicklauncher_focused_indicator.png";
  const string RUNNING_FILE  = Unity.PKGDATADIR
    + "/quicklauncher_running_indicator.png";
  const string HONEYCOMB_MASK_FILE = Unity.PKGDATADIR
    + "/honeycomb-mask.png";
  const string MENU_BG_FILE = Unity.PKGDATADIR
    + "/tight_check_4px.png";

  const uint SHORT_DELAY = 400;
  const uint MEDIUM_DELAY = 800;
  const uint LONG_DELAY = 1600;

  public class LauncherView : Ctk.Bin
  {

    public LauncherModel? model;

    /* the prettys */
    private Ctk.Image icon;
    private Clutter.Texture focused_indicator;
    private Clutter.Texture running_indicator;
    private Gdk.Pixbuf honeycomb_mask;
    private Clutter.Group container;

    private Gee.ArrayList<ShortcutItem> offline_shortcuts;
    private Gee.ArrayList<ShortcutItem> shortcut_actions;
    
    private QuicklistController? quicklist_controller;

    private Ctk.EffectGlow effect_icon_glow;

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

  /* constructors */
    public LauncherView (LauncherModel model)
      {
        /* this is a "view" for a launchermodel object */
        this.model = model;
        this.model.notify_active.connect (this.notify_on_is_running);
        this.model.notify_focused.connect (this.notify_on_is_focused);
        this.model.activated.connect (this.on_activated);
        this.name = "Unity.Quicklauncher.LauncherView-" + this.model.name;

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

        set_reactive (true);
        this.quicklist_controller = null;
      }

    private void load_textures ()
    {
      try
        {
          this.focused_indicator = new Clutter.Texture.from_file (FOCUSED_FILE);
        } catch (Error e)
        {
          this.focused_indicator = new Clutter.Texture ();
          warning ("loading focused indicator failed, %s", e.message);
        }
      this.container.add_actor (this.focused_indicator);

      try
        {
          this.running_indicator = new Clutter.Texture.from_file (RUNNING_FILE);
        } catch (Error e)
        {
          this.running_indicator = new Clutter.Texture ();
          warning ("loading running indicator failed, %s", e.message);
        }
      this.container.add_actor (this.running_indicator);
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
     * re-layouts the various indicators
     */
    private void relayout ()
    {
      float mid_point_y = this.container.height / 2.0f;
      float focus_halfy = this.focused_indicator.height / 2.0f;
      float focus_halfx = container.width + this.focused_indicator.width + 1;

      this.focused_indicator.set_position(focus_halfx,
                                          mid_point_y - focus_halfy);
      this.running_indicator.set_position (0, mid_point_y - focus_halfy);

      this.icon.set_position (6, 0);
    }

    /* animation logic */
    private void throbber_start ()
    {

      effect_icon_glow = new Ctk.EffectGlow ();
      Clutter.Color c = Clutter.Color () {
        red = 255,
        green = 255,
        blue = 255,
        alpha = 255
      };
      effect_icon_glow.set_background_texture (honeycomb_mask);
      effect_icon_glow.set_color (c);
      effect_icon_glow.set_factor (0.0f);
      this.icon.add_effect (effect_icon_glow);
      this.icon.do_queue_redraw ();

      this.anim_throbber = effect_icon_glow.animate (
                          Clutter.AnimationMode.EASE_IN_OUT_SINE, SHORT_DELAY,
                          "factor", 1.0f);

      Signal.connect_after (this.anim_throbber, "completed",
                            (Callback)do_anim_throbber_loop, this);

      GLib.Timeout.add_seconds (8, on_launch_timeout);
    }

    private static void do_anim_throbber_loop (Object sender, LauncherView self)
      requires (self is LauncherView)
    {
      if (self.is_starting)
        {
          // we are still starting so do another loop
          float factor = 0.0f;
          if (self.effect_icon_glow.factor < 0.5)
            {
              factor = 1.0f;
            }

          self.anim_throbber = self.effect_icon_glow.animate (
                                        Clutter.AnimationMode.EASE_IN_OUT_SINE,
                                        SHORT_DELAY,
                                        "factor", factor);
          Signal.connect_after (self.anim_throbber, "completed",
                                (Callback)do_anim_throbber_loop, self);
        }
      else
        {
          // we should fadeout if we are too bright, otherwise remove effect
          if (self.effect_icon_glow.factor >= 0.1)
            {
              self.anim_throbber = self.effect_icon_glow.animate (
                                        Clutter.AnimationMode.EASE_IN_OUT_SINE,
                                        SHORT_DELAY,
                                        "factor", 0.0);
            }
          else
            {
              self.icon.remove_effect (self.effect_icon_glow);
            }
        }
    }

    private void throbber_fadeout ()
    {
      return;
    }

    private void throbber_hide ()
    {
      throbber_fadeout ();
    }


    /* callbacks on model */
    private void on_activated ()
    {
      this.is_starting = false;
    }

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
    private bool on_mouse_enter (Clutter.Event event)
    {
      this.is_hovering = true;
      if (this.quicklist_controller == null)
        {
          this.quicklist_controller = new QuicklistController (this.model.name,
                                                               this,
                                                               this.get_stage () as Clutter.Stage
                                                               );
          this.build_quicklist ();
        }
      if (!this.quicklist_controller.is_label)
        {
          this.quicklist_controller.show_label ();
        }
      return false;
    }

    private bool on_launch_timeout ()
    {
      this.is_starting = false;

      return false;
    }

    private bool on_mouse_leave(Clutter.Event src)
    {
      this.is_hovering = false;
      if (this.quicklist_controller is QuicklistController)
      {
        if (this.quicklist_controller.is_label)
          {
            this.quicklist_controller.close_menu ();
          }
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
        if (this.quicklist_controller.is_label)
          {
            this.quicklist_controller.show_menu ();
          }
      }
      return false;
    }

    /* menu handling */
    private void build_quicklist ()
    {         
      this.offline_shortcuts = this.model.get_menu_shortcuts ();
      this.shortcut_actions = this.model.get_menu_shortcut_actions ();
      foreach (ShortcutItem shortcut in this.offline_shortcuts)
      {
        this.quicklist_controller.add_action (shortcut, true);
      }
      
      foreach (ShortcutItem shortcut in this.shortcut_actions)
      {
        this.quicklist_controller.add_action (shortcut, false);
      }
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
      this.is_starting = true;
    }
  }
}



