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
 * Authored by Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */
using Unity;
using Unity.Quicklauncher;
using Unity.Quicklauncher.Models;
static string? boot_logging_filename = null;

namespace Unity
{
  public class DragDest: Gtk.Window
  {
    public DragDest ()
      {
        Object (type:Gtk.WindowType.TOPLEVEL,
                type_hint:Gdk.WindowTypeHint.DOCK,
                opacity:0.0);
      }

    construct
    {
      ;
    }
  }

  public enum InputState
  {
    NONE,
    FULLSCREEN,
    UNITY,
    ZERO,
  }

  public class ActorBlur : Ctk.Bin
  {
    private Clutter.Clone clone;
    //private Ctk.EffectBlur blur;

    public ActorBlur (Clutter.Actor actor)
    {
      clone = new Clutter.Clone (actor);

      add_actor (clone);
      clone.show ();

      clone.set_position (0, 0);
      //blur = new Ctk.EffectBlur ();
      //blur.set_factor (9f);
      //add_effect (blur);
    }

    construct
    {

    }
  }

  public class Plugin : Object, Shell
  {
    /* Signals */
    public signal void window_minimized (Plugin plugin, Mutter.Window window);
    public signal void window_maximized (Plugin        plugin,
                                         Mutter.Window window,
                                         int           x,
                                         int           y,
                                         int           width,
                                         int           height);
    public signal void window_unmaximized (Plugin        plugin,
                                           Mutter.Window window,
                                           int           x,
                                           int           y,
                                           int           width,
                                           int           height);
    public signal void window_mapped (Plugin plugin, Mutter.Window window);
    public signal void window_destroyed (Plugin plugin, Mutter.Window window);
    public signal void window_kill_effect (Plugin        plugin,
                                           Mutter.Window window,
                                           ulong         events);

    public signal void workspace_switch_event (Plugin plugin,
                                               List<Mutter.Window> windows,
                                               int from,
                                               int to,
                                               int direction);

    public signal void restore_input_region (bool fullscreen);

    /* Properties */
    private Mutter.Plugin? _plugin;
    public  Mutter.Plugin? plugin {
      get { return _plugin; }
      set { _plugin = value; this.real_construct (); }
    }

    public bool menus_swallow_events { get { return false; } }

    public bool expose_showing { get { return expose_manager.expose_showing; } }

    private static const int PANEL_HEIGHT        = 23;
    private static const int QUICKLAUNCHER_WIDTH = 58;

    private Clutter.Stage    stage;
    private Application      app;
    private WindowManagement wm;
    private Maximus          maximus;

    /* Unity Components */
    private Background         background;
    private ExposeManager      expose_manager;
    private Quicklauncher.View quicklauncher;
    private Places.Controller  places_controller;
    private Places.View        places;
    private Panel.View         panel;
    private ActorBlur          actor_blur;
    private Clutter.Rectangle  dark_box;

    private bool     places_enabled = false;

    private DragDest drag_dest;
    private bool     places_showing;
    private bool     _fullscreen_obstruction;
    private InputState last_input_state = InputState.NONE;

    private Gee.ArrayList<Object> fullscreen_requests;

    private bool fullscreen_obstruction
      {
        get {
          return _fullscreen_obstruction;
        }
        set {
          _fullscreen_obstruction = value;
          ensure_input_region ();
        }
      }

    private bool grab_enabled = false;
    private DBus.Connection screensaver_conn;
    private dynamic DBus.Object screensaver;

    construct
    {
      Unity.global_shell = this;
      Unity.TimelineLogger.get_default(); // just inits the timer for logging
      // attempt to get a boot logging filename
      boot_logging_filename = Environment.get_variable ("UNITY_BOOTLOG_FILENAME");
      if (boot_logging_filename != null)
        {
          Unity.is_logging = true;
        }
      else
        {
          Unity.is_logging = false;
        }
      START_FUNCTION ();
      string[] args = { "mutter" };

      LOGGER_START_PROCESS ("ctk_init");
      Ctk.init_after (ref args);
      LOGGER_END_PROCESS ("ctk_init");

      Gtk.Settings.get_default ().gtk_icon_theme_name = "ubuntu-mono-dark";

      /* Unique instancing */
      LOGGER_START_PROCESS ("unity_application_constructor");
      this.app = new Unity.Application ();
      this.app.shell = this;
      LOGGER_END_PROCESS ("unity_application_constructor");

      try
        {
          this.screensaver_conn = DBus.Bus.get (DBus.BusType.SESSION);
          this.screensaver = this.screensaver_conn.get_object ("org.gnome.ScreenSaver", "/org/gnome/ScreenSaver", "org.gnome.ScreenSaver");
          this.screensaver.ActiveChanged += got_screensaver_changed;
        }
      catch (Error e)
        {
          warning (e.message);
        }
      END_FUNCTION ();
    }

    private void real_construct ()
    {
      START_FUNCTION ();
      this.wm = new WindowManagement (this);
      this.maximus = new Maximus ();

      fullscreen_requests = new Gee.ArrayList<Object> ();

      this.stage = (Clutter.Stage)this.plugin.get_stage ();
      this.stage.actor_added.connect   ((a) => { ensure_input_region (); });
      this.stage.actor_removed.connect ((a) => { ensure_input_region (); });

      this.drag_dest = new DragDest ();
      this.drag_dest.show ();
      Gtk.TargetEntry[] target_list =
        {
          Gtk.TargetEntry () {target="STRING", flags=0,
                              info=Unity.dnd_targets.TARGET_STRING },
          Gtk.TargetEntry () {target="text/plain", flags=0,
                              info=Unity.dnd_targets.TARGET_STRING },
          Gtk.TargetEntry () {target="text/uri-list", flags=0,
                              info=Unity.dnd_targets.TARGET_URL },
          Gtk.TargetEntry () {target="x-url/http",
                              flags=0, info=Unity.dnd_targets.TARGET_URL },
          Gtk.TargetEntry () {target="x-url/ftp",
                              flags=0, info=Unity.dnd_targets.TARGET_URL },
          Gtk.TargetEntry () {target="_NETSCAPE_URL", flags=0,
                              info=Unity.dnd_targets.TARGET_URL }
        };

      Ctk.dnd_init ((Gtk.Widget)this.drag_dest, target_list);

      this.places_enabled = envvar_is_enabled ("UNITY_ENABLE_PLACES");

      this.background = new Background ();
      this.stage.add_actor (this.background);
      this.background.lower_bottom ();

      Clutter.Group window_group = (Clutter.Group) this.plugin.get_window_group ();

      /* Allows us to activate windows, essential as we are the WM */
      Launcher.Application.set_window_activate_func (this.on_window_activated,
                                                     this.plugin);

      this.quicklauncher = new Quicklauncher.View (this);
      this.quicklauncher.opacity = 0;

      this.expose_manager = new ExposeManager (this, quicklauncher);
      this.expose_manager.hovered_opacity = 255;
      this.expose_manager.unhovered_opacity = 255;
      this.expose_manager.darken = 25;
      this.expose_manager.right_buffer = 10;
      this.expose_manager.top_buffer = this.expose_manager.bottom_buffer = 20;

      this.expose_manager.coverflow = false;

      this.quicklauncher.manager.active_launcher_changed.connect (on_launcher_changed_event);

      window_group.add_actor (this.quicklauncher);
      window_group.raise_child (this.quicklauncher,
                                this.plugin.get_normal_window_group ());
      this.quicklauncher.animate (Clutter.AnimationMode.EASE_IN_SINE, 400,
                                  "opacity", 255);

      if (this.places_enabled)
        {
          this.places_controller = new Places.Controller (this);
          this.places = this.places_controller.get_view ();

          window_group.add_actor (this.places);
          window_group.raise_child (this.places,
                                    this.quicklauncher);
          this.places.opacity = 0;
          this.places.reactive = false;
          this.places.hide ();
          this.places_showing = false;
        }

      this.panel = new Panel.View (this);
      window_group.add_actor (this.panel);
      window_group.raise_child (this.panel,
                                this.quicklauncher);
      this.panel.show ();

      this.stage.notify["width"].connect (this.relayout);
      this.stage.notify["height"].connect (this.relayout);

      this.relayout ();
      END_FUNCTION ();

      if (boot_logging_filename != null)
        {
          Timeout.add_seconds (5, () => {
            Unity.TimelineLogger.get_default().write_log (boot_logging_filename);
            return false;
          });
        }

      this.ensure_input_region ();

      Wnck.Screen.get_default ().active_window_changed.connect (on_active_window_changed);

      if (Wnck.Screen.get_default ().get_active_window () != null)
        Wnck.Screen.get_default ().get_active_window ().state_changed.connect (on_active_window_state_changed);
    }

    private static void on_window_activated (Wnck.Window  window,
                                             uint32       timestamp,
                                             void        *data)
    {
      Mutter.Plugin plugin = data as Mutter.Plugin;

      unowned GLib.List<Mutter.Window> mutter_windows = plugin.get_windows ();
      foreach (Mutter.Window w in mutter_windows)
        {
          ulong xid = (ulong) Mutter.MetaWindow.get_xwindow (w.get_meta_window ());
          if (window.get_xid () == xid)
            {
              unowned Mutter.MetaWindow win  = w.get_meta_window ();

              Mutter.MetaWorkspace.activate (Mutter.MetaWindow.get_workspace(win),
                                             timestamp);
              Mutter.MetaWindow.activate (win, timestamp);
              break;
            }
        }
    }

    private void on_active_window_state_changed (Wnck.WindowState change_mask, Wnck.WindowState new_state)
    {
      check_fullscreen_obstruction ();
    }

    private void on_active_window_changed (Wnck.Window? previous_window)
    {
      if (previous_window != null)
        previous_window.state_changed.disconnect (on_active_window_state_changed);


      Wnck.Window current = Wnck.Screen.get_default ().get_active_window ();
      if (current == null)
        return;

      current.state_changed.connect (on_active_window_state_changed);

      check_fullscreen_obstruction ();
    }

    private void on_launcher_changed_event (LauncherView? last, LauncherView? current)
    {
      if (last != null)
        {
          last.menu_opened.disconnect (on_launcher_menu_opened);
          last.menu_closed.disconnect (on_launcher_menu_closed);
        }

      if (current != null)
        {
          current.menu_opened.connect (on_launcher_menu_opened);
          current.menu_closed.connect (on_launcher_menu_closed);
        }

      check_fullscreen_obstruction ();
    }

    private void on_launcher_menu_opened (LauncherView sender)
    {
      if (sender != quicklauncher.manager.active_launcher || sender == null)
        return;

      if (sender.model is ApplicationModel && sender.model.is_active)
        {
          if (QuicklistController.get_default ().menu_is_open () &&
              !QuicklistController.get_default ().menu.close_on_leave)
            expose_windows ((sender.model as ApplicationModel).windows);
        }
    }

    private void on_launcher_menu_closed (LauncherView sender)
    {
      if (sender != quicklauncher.manager.active_launcher)
        return;

      dexpose_windows ();
    }

    private void got_screensaver_changed (dynamic DBus.Object screensaver, bool changed)
    {
      if (changed)
        {
          this.quicklauncher.hide ();
          this.panel.hide ();
          var menu = Unity.Quicklauncher.QuicklistController.get_default ();
          if (menu.menu_is_open ())
            menu.close_menu ();
          fullscreen_obstruction = true;
        }
      else
        {
          this.quicklauncher.show ();
          this.panel.show ();
          fullscreen_obstruction = false;
        }
    }

    bool window_is_obstructing (Wnck.Window window)
    {
      if (window.is_fullscreen ())
          return true;

      /* Sometimes we're not getting the fullscreen hint updating fast enough
       * but the geometry seems to mostly be in sync. Seeing if the window is
       * size-wise fullscreen seems to give us a good fallback when the props
       * aren't in-sync.
       * The -2.0 is because we have some variation when converting float to int
       * from the stage.
       */
      int x, y, w, h;
      window.get_geometry (out x, out y, out w, out h);

      if (w >= (int)this.stage.width - 2.0
          && h >= (int)this.stage.height - 2.0)
          return true;

      return false;
    }


    void check_fullscreen_obstruction ()
    {
      Wnck.Window current = Wnck.Screen.get_default ().get_active_window ();
      if (current == null)
        return;

      if (window_is_obstructing (current))
        {
          this.quicklauncher.animate (Clutter.AnimationMode.EASE_IN_SINE, 200, "x", -100f);
          this.panel.animate (Clutter.AnimationMode.EASE_IN_SINE, 200, "opacity", 0);
          fullscreen_obstruction = true;
        }
      else
        {
          this.quicklauncher.animate (Clutter.AnimationMode.EASE_IN_SINE, 200, "x", 0f);
          this.panel.animate (Clutter.AnimationMode.EASE_IN_SINE, 200, "opacity", 255);
          fullscreen_obstruction = false;
        }
    }

    private void relayout ()
    {
      START_FUNCTION ();
      float width, height;

      this.stage.get_size (out width, out height);

      this.drag_dest.resize (this.QUICKLAUNCHER_WIDTH,
                             (int)height - this.PANEL_HEIGHT);
      this.drag_dest.move (0, this.PANEL_HEIGHT);

      this.background.set_size (width, height);
      this.background.set_position (0, 0);

      this.quicklauncher.set_size (this.QUICKLAUNCHER_WIDTH,
                                   (height-this.PANEL_HEIGHT));
      this.quicklauncher.set_position (0, this.PANEL_HEIGHT);
      this.quicklauncher.set_clip (0, 0,
                                   this.QUICKLAUNCHER_WIDTH,
                                   height-this.PANEL_HEIGHT);
      Utils.set_strut ((Gtk.Window)this.drag_dest,
                       this.QUICKLAUNCHER_WIDTH - 1, 0, (uint32)height,
                       PANEL_HEIGHT, 0, (uint32)width);

      if (this.places_enabled)
        {
          this.places.set_size (width, height);
          this.places.set_position (0, 0);
        }

      this.panel.set_size (width, 24);
      this.panel.set_position (0, 0);

      /* Leaving this here to remind me that we need to use these when
       * there are fullscreen windows etc
       * this.plugin.set_stage_input_region (uint region);
       * this.plugin.set_stage_reactive (true);
       */
      END_FUNCTION ();
    }

    public void add_fullscreen_request (Object o)
    {
      fullscreen_requests.add (o);
      ensure_input_region ();
    }

    public bool remove_fullscreen_request (Object o)
    {
      bool result = fullscreen_requests.remove (o);
      ensure_input_region ();
      return result;
    }

    public void ensure_input_region ()
    {
      if (fullscreen_obstruction)
        {
          if (last_input_state == InputState.ZERO)
            return;
          last_input_state = InputState.ZERO;
          this.plugin.set_stage_input_area(0, 0, 0, 0);

          this.grab_keyboard (false, Clutter.get_current_event_time ());
        }
      else if (fullscreen_requests.size > 0 || places_showing)
        {
          // Fullscreen required
          if (last_input_state == InputState.FULLSCREEN)
            return;

          last_input_state = InputState.FULLSCREEN;
          this.restore_input_region (true);

          this.stage.set_key_focus (null as Clutter.Actor);
          this.grab_keyboard (true, Clutter.get_current_event_time ());
        }
      else
        {
          // Unity mode requred
          if (last_input_state == InputState.UNITY)
            return;

          last_input_state = InputState.UNITY;
          this.restore_input_region (false);

          this.grab_keyboard (false, Clutter.get_current_event_time ());
        }
    }

    /*
     * SHELL IMPLEMENTATION
     */

    public void show_window_picker ()
    {
      if (expose_manager.expose_showing == true)
        {
          this.dexpose_windows ();
          return;
        }

      GLib.SList <Wnck.Window> windows = null;

      foreach (Wnck.Window w in Wnck.Screen.get_default ().get_windows ())
        {
          switch (w.get_window_type ())
            {
            case Wnck.WindowType.NORMAL:
            case Wnck.WindowType.DIALOG:
            case Wnck.WindowType.UTILITY:
              windows.append (w);
              break;

            default:
              break;
            }
        }

      this.expose_windows (windows, 80);
    }

    public Clutter.Stage get_stage ()
    {
      return this.stage;
    }

    public ShellMode get_mode ()
    {
      return ShellMode.UNDERLAY;
    }

    public int get_indicators_width ()
    {
      return this.panel.get_indicators_width ();
    }

    public void expose_windows (GLib.SList<Wnck.Window> windows,
                                int left_buffer = 250)
    {
      expose_manager.left_buffer = left_buffer;
      expose_manager.start_expose (windows);
    }

    public void dexpose_windows ()
    {
      expose_manager.end_expose ();
    }

    public void show_unity ()
    {
      if (this.places_enabled == false)
        {
          var screen = Wnck.Screen.get_default ();

          screen.toggle_showing_desktop (!screen.get_showing_desktop ());
          return;
        }
      if (this.places_showing)
        {
          this.places_showing = false;

          this.places.hide ();
          this.places.opacity = 0;
          this.places.reactive = false;

          this.actor_blur.destroy ();
          this.dark_box.destroy ();
          this.panel.set_indicator_mode (false);

          this.ensure_input_region ();
        }
      else
        {
          this.places_showing = true;

          this.places.show ();
          this.places.opacity = 255;
          this.places.reactive = true;

          this.actor_blur = new ActorBlur (this.plugin.get_normal_window_group ());

          (this.plugin.get_window_group () as Clutter.Container).add_actor (this.actor_blur);
          (this.actor_blur as Clutter.Actor).raise (this.plugin.get_normal_window_group ());

          this.actor_blur.set_position (0, 0);

          this.dark_box = new Clutter.Rectangle.with_color ({0, 0, 0, 255});

          (this.plugin.get_window_group () as Clutter.Container).add_actor (this.dark_box);
          this.dark_box.raise (this.actor_blur);

          this.dark_box.set_position (0, 0);
          this.dark_box.set_size (this.stage.width, this.stage.height);

          this.dark_box.show ();
          this.actor_blur.show ();
          this.panel.set_indicator_mode (true);

          this.ensure_input_region ();

          new X.Display (null).flush ();

          this.dark_box.opacity = 0;
          this.actor_blur.opacity = 255;

          this.dark_box.animate   (Clutter.AnimationMode.EASE_IN_SINE, 250, "opacity", 180);
        }
    }

    public void grab_keyboard (bool grab, uint32 timestamp)
    {
      if (this.grab_enabled == grab)
        return;

      if (grab)
        {
          this.plugin.begin_modal (Utils.get_stage_window (this.stage),
                                   0,
                                   0,
                                   timestamp);

        }
      else
        {
          this.plugin.end_modal (timestamp);
        }

      this.grab_enabled = grab;
    }

    private bool envvar_is_enabled (string name)
    {
      return (Environment.get_variable (name) != null);
    }

    /*
     * MUTTER PLUGIN HOOKS
     */
    public void minimize (Mutter.Window window)
    {
      this.window_minimized (this, window);
    }

    public void maximize (Mutter.Window window,
                          int           x,
                          int           y,
                          int           width,
                          int           height)
    {
      this.window_maximized (this, window, x, y, width, height);
    }

    public void unmaximize (Mutter.Window window,
                            int           x,
                            int           y,
                            int           width,
                            int           height)
    {
      this.window_unmaximized (this, window, x, y, width, height);
    }

    public void map (Mutter.Window window)
    {
      this.maximus.process_window (window);
      this.window_mapped (this, window);
    }

    public void destroy (Mutter.Window window)
    {
      this.window_destroyed (this, window);
    }

    public void switch_workspace (List<Mutter.Window> windows,
                                  int                 from,
                                  int                 to,
                                  int                 direction)
    {
      this.workspace_switch_event (this, windows, from, to, direction);
    }

    public void kill_effect (Mutter.Window window, ulong events)
    {
      this.window_kill_effect (this, window, events);
    }

    public int get_panel_height ()
    {
      return PANEL_HEIGHT;
    }

    public int get_launcher_width ()
    {
      return QUICKLAUNCHER_WIDTH;
    }
  }
}
