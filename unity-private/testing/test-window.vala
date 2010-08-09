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

using Unity.Testing;

namespace Unity.Testing
{
  public class Window : Gtk.Window, Shell
  {
    public bool menus_swallow_events { get { return true; } }
    public bool is_popup     { get; construct; }
    public int  popup_width  { get; construct; }
    public int  popup_height { get; construct; }

    private Wnck.Screen wnck_screen;
    private Workarea    workarea_size;

    public  GtkClutter.Embed gtk_clutter;
    public  Clutter.Stage    stage;
    private bool             is_showing;

    private Background          background;
    private Launcher.Launcher   launcher;
    private Panel.View          panel;
    private Places.Controller   controller;
    private Unity.Places.View   places;
    private Gesture.Dispatcher  gesture_dispatcher;

    private bool showing_places;

    public Window (bool popup, int width, int height)
    {
      Object(is_popup: popup, popup_width: width, popup_height: height);
    }

    construct
    {
      START_FUNCTION ();
      Unity.global_shell = this;
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
      LOGGER_START_PROCESS ("unity_underlay_window_realize");
      this.realize ();
      LOGGER_END_PROCESS ("unity_underlay_window_realize");

      this.gtk_clutter = new GtkClutter.Embed ();
      this.add (this.gtk_clutter);
      LOGGER_START_PROCESS ("gtk_clutter_realize");
      this.gtk_clutter.realize ();
      LOGGER_END_PROCESS ("gtk_clutter_realize");
      //setup dnd
      Gtk.TargetEntry[] target_list = {
        Gtk.TargetEntry () {target="STRING", flags=0, info=Unity.dnd_targets.TARGET_STRING },
        Gtk.TargetEntry () {target="text/plain", flags=0, info=Unity.dnd_targets.TARGET_STRING },
        Gtk.TargetEntry () {target="text/uri-list", flags=0, info=Unity.dnd_targets.TARGET_URL },
        Gtk.TargetEntry () {target="x-url/http", flags=0, info=Unity.dnd_targets.TARGET_URL },
        Gtk.TargetEntry () {target="x-url/ftp", flags=0, info=Unity.dnd_targets.TARGET_URL },
        Gtk.TargetEntry () {target="_NETSCAPE_URL", flags=0, info=Unity.dnd_targets.TARGET_URL }
      };

      LOGGER_START_PROCESS ("ctk_dnd_init");
      Ctk.dnd_init (this.gtk_clutter, target_list);
      LOGGER_END_PROCESS ("ctk_dnd_init");

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

      this.launcher = new Launcher.Launcher (this);
      this.stage.add_actor (this.launcher.get_container ());
      (this.launcher.get_container () as Ctk.Bin).add_actor (this.launcher.get_view ());
      this.launcher.get_container ().show ();

      this.controller = new Places.Controller (this);
      this.places = this.controller.get_view ();
      this.stage.add_actor (this.places);
      this.places.opacity = 0;
      this.showing_places = false;

      this.panel = new Panel.View (this);
      this.stage.add_actor (this.panel);
      this.panel.show ();

      /* Layout everything */
      this.move (0, 0);
      this.relayout ();

      /* Window management */
      this.wnck_screen = Wnck.Screen.get_default ();
      if (!this.is_popup)
        {
          this.wnck_screen.active_window_changed.connect (this.on_active_window_changed);
        }
      
      gesture_dispatcher = new Gesture.XCBDispatcher ();

      END_FUNCTION ();
    }

    public uint32 get_current_time ()
    {
      return Clutter.get_current_event_time ();
    }

    public void init_test_mode ()
    {
      /* Make a plain background */
      stage.color = { 148, 86, 99, 255 };
      background.opacity = 0;

      /* Font stuff */
      unowned Gtk.Settings settings = Gtk.Settings.get_default ();
      settings.gtk_xft_dpi = 96 * 1024;
      settings.gtk_font_name = "Sans 10";
      settings.gtk_icon_theme_name = "ubuntu-mono-dark";
      settings.gtk_theme_name = "Ambiance";
    }

    private void relayout ()
    {
      int x, y, width, height;
      float ql_width;
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

      ql_width = 58;
      this.resize (width, height);
      this.stage.set_size (width, height);

      if (!this.is_popup)
        Utils.set_strut ((Gtk.Window)this,
                         (uint)(ql_width), 0, height,
                         24, 0, width);

      /* Update component layouts */
      this.background.set_position (0, 0);
      this.background.set_size (width, height);

      this.launcher.get_container ().set_size (ql_width,
                                          height - Unity.Panel.PANEL_HEIGHT);
      this.launcher.get_container ().set_position (0, Unity.Panel.PANEL_HEIGHT);

      this.launcher.get_container ().set_clip (0, 0,
                                          ql_width, height);

      this.places.set_size (width-ql_width, height);
      this.places.set_position (ql_width, 0);

      this.panel.set_size (width, Unity.Panel.PANEL_HEIGHT);
      this.panel.set_position (0, 0);
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
    private void on_active_window_changed (Wnck.Window? previous_window)
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

    public void get_window_details (uint32   xid,
                                    out bool allows_resize,
                                    out bool is_maximised)
    {
      allows_resize = true;
      debug ("This target does not support getting window details");
    }

    public void do_window_action (uint32 xid, WindowAction action)
    {
      debug ("This target does not support window actions");
    }

    public void show_window_picker ()
    {
      this.show_unity ();
    }

    public void grab_keyboard (bool grab, uint32 timestamp)
    {
      /* Doesn't work with clutter-gtk */
    }

    public Clutter.Stage get_stage ()
    {
      return this.stage;
    }

    public ShellMode get_mode ()
    {
      return showing_places ? ShellMode.DASH : ShellMode.MINIMIZED;
    }

    public void show_unity ()
    {
      if (this.showing_places)
        {
          /* We're already showing, so we hide */
          this.showing_places = false;
          this.panel.set_indicator_mode (false);
          this.background.opacity = 255;
          this.places.opacity = 0;

          places.hidden ();
        }
      else
        {
          this.showing_places = true;
          this.panel.set_indicator_mode (true);
          this.background.opacity = 160;
          this.places.opacity = 255;

          places.shown ();
        }

      mode_changed (showing_places ? ShellMode.DASH : ShellMode.MINIMIZED);

      this.places.do_queue_redraw ();
    }

    public void hide_unity ()
    {
      if (showing_places == true)
        {
          showing_places = false;
          panel.set_indicator_mode (false);
          background.opacity = 255;
          places.opacity = 0;

          places.hidden ();

          mode_changed (ShellMode.MINIMIZED);
        }
    }

    public void about_to_show_places ()
    {
      places.about_to_show ();
    }

    public int get_indicators_width ()
    {
      return this.panel.get_indicators_width ();
    }

    public int get_launcher_width_foobar ()
    {
      return (int) this.launcher.get_width ();
    }

    public int get_panel_height_foobar ()
    {
      return (int) this.panel.get_panel_height ();
    }

    public void add_fullscreen_request (Object o)
    {
      return;
    }

    public bool remove_fullscreen_request (Object o)
    {
      return false;
    }

    public void ensure_input_region ()
    {
      /* GOOD JOB!!! */
      return;
    }

    public void close_xids (Array<uint32> xids)
    {
      for (int i = 0; i < xids.length; i++)
        {
          uint32 xid = xids.index (i);
          Wnck.Window window = Wnck.Window.get (xid);
          if (window is Wnck.Window)
            window.close (Clutter.get_current_event_time ());
        }
    }

    public void	expose_xids (Array<uint32> xids)
		{
      //null op in popup
		}

		public void stop_expose ()
		{
			//null op in popup
		}

    public void show_window (uint32 xid)
    {
      Wnck.Window window = Wnck.Window.get (xid);
      if (window is Wnck.Window)
        window.activate (Clutter.get_current_event_time ());
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
