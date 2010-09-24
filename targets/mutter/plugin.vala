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

using GConf;
using Unity;
using Unity.Testing;

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
      this.set_accept_focus (false);
    }
  }

  public enum InputState
  {
    NONE,
    FULLSCREEN,
    UNITY,
    ZERO,
  }

  /*
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

  */
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
    public signal void kill_window_effects (Plugin plugin, Mutter.Window window);
    public signal void kill_switch_workspace (Plugin plugin);

    public signal void workspace_switch_event (Plugin plugin,
                                               int from,
                                               int to,
                                               int direction);

    public signal void restore_input_region (bool fullscreen);

    /* Properties */
    private Mutter.Plugin? _plugin;
    public  Mutter.Plugin? plugin {
      get { return _plugin; }
      set { _plugin = value; Idle.add (real_construct); }
    }
    private bool _super_key_enable=true;
    public bool super_key_enable {
      get { return _super_key_enable; }
      set { _super_key_enable = value; }
    }

    public ExposeManager expose_manager { get; private set; }
    
    public bool menus_swallow_events { get { return false; } }

    private bool _super_key_active = false;
    public bool super_key_active {
      get { return _super_key_active; }
      set { _super_key_active = value; }
    }
    public bool is_starting {get; set;}

    public bool expose_showing { get { return expose_manager.expose_showing; } }

    private static const int PANEL_HEIGHT        =  24;
    private static const int QUICKLAUNCHER_WIDTH = 58;
    private static const string UNDECORATED_HINT = "UNDECORATED_HINT";

    public Gee.ArrayList<Background> backgrounds;
    public Gdk.Rectangle primary_monitor;

    private Clutter.Stage    stage;
    private Application      app;
    private WindowManagement wm;
    private Maximus          maximus;

    /* Unity Components */
    private SpacesManager      spaces_manager;
    private Launcher.Launcher  launcher;
    private Places.Controller  places_controller;
    private Places.View        places;
    private Panel.View         panel;
    private Clutter.Rectangle  dark_box;
    private unowned Mutter.MetaWindow?  focus_window = null;
    private unowned Mutter.MetaDisplay? display = null;

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

    public Gesture.Dispatcher gesture_dispatcher;
    private Gesture.Type active_gesture_type = Gesture.Type.NONE;

    /* Pinch info */
    /* private float start_pinch_radius = 0.0f; */
    private unowned Mutter.Window? resize_window = null;
    /*private float   resize_last_x1 = 0.0f;
    private float   resize_last_y1 = 0.0f;
    private float   resize_last_x2 = 0.0f;
    private float   resize_last_y2 = 0.0f;*/

    /* Pan info */
    private unowned Mutter.Window? start_pan_window = null;
    private unowned Clutter.Rectangle? start_frame_rect = null;
    private float last_pan_x_root = 0.0f;
    private float last_pan_maximised_x_root = 0.0f;
    private enum MaximizeType {
      NONE,
      FULL,
      LEFT,
      RIGHT
    }
    private MaximizeType maximize_type = MaximizeType.NONE;

    private enum ExposeType {
      NONE,
      APPLICATION,
      WINDOWS,
      WORKSPACE
    }
    private ExposeType expose_type = ExposeType.NONE;

    /* const */
    private const string GCONF_DIR = "/desktop/unity/launcher";
    private const string GCONF_SUPER_KEY_ENABLE_KEY = "super_key_enable";

    construct
    {
      is_starting = true;
      fullscreen_requests = new Gee.ArrayList<Object> ();
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
          this.screensaver.ActiveChanged.connect (got_screensaver_changed);
        }
      catch (GLib.Error e)
        {
          warning (e.message);
        }

      this.wm = new WindowManagement (this);
      this.maximus = new Maximus ();

      (Clutter.Stage.get_default () as Clutter.Stage).color = { 0, 0, 0, 255 };

      END_FUNCTION ();
    }

    private bool real_construct ()
    {
      START_FUNCTION ();

      Clutter.set_gl_picking_enabled (false);

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

      Clutter.Group window_group = (Clutter.Group) this.plugin.get_window_group ();

      /* we need to hook into the super key bound by mutter for g-shell.
         don't ask me why mutter binds things for g-shell explictly...
         */
      var gc = GConf.Client.get_default();
      Mutter.MetaDisplay display = Mutter.MetaScreen.get_display (plugin.get_screen ());

      try {
      	  super_key_enable = gc.get_bool(GCONF_DIR + "/" + GCONF_SUPER_KEY_ENABLE_KEY);
      } catch (GLib.Error e) {
          super_key_enable = true;
          warning("Cannot find super_key_enable gconf key");
      }
      try {
          gc.add_dir(GCONF_DIR, GConf.ClientPreloadType.ONELEVEL);
      	  gc.notify_add(GCONF_DIR + "/" + GCONF_SUPER_KEY_ENABLE_KEY, this.gconf_super_key_enable_cb);
      } catch (GLib.Error e) {
          warning("Cannot set gconf callback function of super_key_enable");
      }

      display.overlay_key_down.connect (() => {
          if (super_key_enable) {
              super_key_active = true;
          }
      });

      display.overlay_key.connect (() => {
          super_key_active = false;
      });

      display.overlay_key_with_modifier.connect ((keysym) => {
        super_key_modifier_release (keysym);
      });

      display.overlay_key_with_modifier_down.connect ((keysym) => {
          if (super_key_enable) {
            super_key_modifier_press (keysym);
          }
      });

      /* Setup the backgrounds */
      unowned Gdk.Screen screen = Gdk.Screen.get_default ();
      backgrounds = new Gee.ArrayList<Background> ();

      /* Connect to interestng signals */
      screen.monitors_changed.connect (relayout);
      screen.size_changed.connect (relayout);

      this.launcher = new Launcher.Launcher (this);
      this.launcher.get_view ().opacity = 0;

      this.spaces_manager = new SpacesManager (this);
      this.spaces_manager.set_padding (50, 50, get_launcher_width_foobar () + 50, 50);
      this.launcher.model.add (spaces_manager.button);

      this.expose_manager = new ExposeManager (this, launcher);
      this.expose_manager.hovered_opacity = 255;
      this.expose_manager.unhovered_opacity = 255;
      this.expose_manager.darken = 25;
      this.expose_manager.right_buffer = 10;
      this.expose_manager.top_buffer = this.expose_manager.bottom_buffer = 20;

      this.expose_manager.coverflow = false;

      window_group.add_actor (this.launcher.get_container ());
      (this.launcher.get_container () as Ctk.Bin).add_actor (this.launcher.get_view ());
      window_group.raise_child (this.launcher.get_container (),
                                this.plugin.get_normal_window_group ());
      this.launcher.get_view ().animate (Clutter.AnimationMode.EASE_IN_SINE, 400,
                                  "opacity", 255);

      this.places_controller = new Places.Controller (this);
      this.places = this.places_controller.get_view ();

      window_group.add_actor (this.places);
      window_group.raise_child (this.places,
                                this.launcher.get_container ());
      this.places.opacity = 0;
      this.places.reactive = false;
      this.places.hide ();
      this.places_showing = false;

      this.panel = new Panel.View (this);
      window_group.add_actor (this.panel);
      window_group.raise_child (this.panel,
                                this.launcher.get_container ());
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

      gesture_dispatcher = new Gesture.XCBDispatcher ();
      gesture_dispatcher.gesture.connect (on_gesture_received);
      
      this.ensure_input_region ();
      GLib.Idle.add (() => { is_starting = false; return false; });
      return false;
      
    }

    private void gconf_super_key_enable_cb(GConf.Client gc, uint cxnid, GConf.Entry entry) {
      bool new_value = true;
      try {
          new_value = gc.get_bool(GCONF_DIR + "/" + GCONF_SUPER_KEY_ENABLE_KEY);
      } catch (GLib.Error e) {
          new_value = true;
      }
      super_key_enable = new_value;
    }

    private void on_focus_window_changed ()
    {
      check_fullscreen_obstruction ();

      if (focus_window != null)
        {
          focus_window.notify["fullscreen"].disconnect (on_focus_window_fullscreen_changed);
        }

      display.get ("focus-window", ref focus_window);
      focus_window.notify["fullscreen"].connect (on_focus_window_fullscreen_changed);
    }

    private void on_focus_window_fullscreen_changed ()
    {
      check_fullscreen_obstruction ();
    }

    private void got_screensaver_changed (dynamic DBus.Object screensaver, bool changed)
    {
      if (changed)
        {
          this.launcher.get_container ().hide ();
          this.panel.hide ();
          var menu = Unity.Launcher.QuicklistController.get_current_menu ();
          if (menu.is_menu_open ())
            menu.state = Unity.Launcher.QuicklistControllerState.CLOSED;

          fullscreen_obstruction = true;
        }
      else
        {
          this.launcher.get_container ().show ();
          this.panel.show ();
          fullscreen_obstruction = false;
        }
    }

    public uint32 get_current_time ()
    {
      return Mutter.MetaDisplay.get_current_time (Mutter.MetaScreen.get_display (plugin.get_screen ()));
    }

    void check_fullscreen_obstruction ()
    {
      Mutter.Window focus = null;
      bool fullscreen = false;

      // prevent segfault when mutter beats us to the initialization punch
      if (!(launcher is Launcher.Launcher) || !(panel is Clutter.Actor))
        return;

      unowned GLib.List<Mutter.Window> mutter_windows = plugin.get_windows ();
      foreach (Mutter.Window w in mutter_windows)
        {
          unowned Mutter.MetaWindow meta = w.get_meta_window ();

          if (meta != null && Mutter.MetaWindow.has_focus (w.get_meta_window ()))
            {
              focus = w;
              break;
            }
        }

      if (focus == null)
        return;

      (focus.get_meta_window () as GLib.Object).get ("fullscreen", ref fullscreen);
      (this.launcher.get_container () as Launcher.LauncherContainer).cache.invalidate_texture_cache ();
      this.panel.cache.invalidate_texture_cache ();

      Clutter.Animation? anim;
      Clutter.Animation? panim;
      if (fullscreen)
        {
          anim = this.launcher.get_container ().animate (Clutter.AnimationMode.EASE_IN_SINE, 200, "x", -100f);
          panim = this.panel.animate (Clutter.AnimationMode.EASE_IN_SINE, 200, "opacity", 0);
          fullscreen_obstruction = true;
        }
      else
        {
          anim = this.launcher.get_container ().animate (Clutter.AnimationMode.EASE_IN_SINE, 200, "x", 0f);
          panim = this.panel.animate (Clutter.AnimationMode.EASE_IN_SINE, 200, "opacity", 255);
          fullscreen_obstruction = false;
        }

      if (anim is Clutter.Animation)
        {
          anim.completed.connect (()=> {
            (this.launcher.get_container () as Launcher.LauncherContainer).cache.update_texture_cache ();
          });
        }

      if (panim is Clutter.Animation)
        {
          panim.completed.connect (() => { this.panel.cache.update_texture_cache (); });
        }
    }

    private void refresh_n_backgrounds (int n_monitors)
    {
      int size = backgrounds.size;

      if (size == n_monitors)
        return;
      else if (size < n_monitors)
        {
          for (int i = 0; i < n_monitors - size; i++)
            {
              var bg = new Background ();
              backgrounds.add (bg);
              stage.add_actor (bg);
              bg.lower_bottom ();
              bg.opacity = 0;
              bg.show ();
              bg.animate (Clutter.AnimationMode.EASE_IN_QUAD, 2000,
                          "opacity", 255);
            }
        }
      else
        {
          for (int i = 0; i < size - n_monitors; i++)
            {
              var bg = backgrounds.get (0);
              if (bg is Clutter.Actor)
                {
                  backgrounds.remove (bg);
                  stage.remove_actor (bg);
                }
            }
        }
    }
    private void relayout ()
    {
      START_FUNCTION ();

      unowned Gdk.Screen screen = Gdk.Screen.get_default ();
      int x, y, width, height;

      /* Figure out what should be the right size and location of Unity */
      /* FIXME: This needs to always be monitor 0 right now as it doesn't
       * seem possible to have panels on a vertical edge of a monitor unless
       * it's the first or last monitor :(
       */
      screen.get_monitor_geometry (0, // Should be screen.get_primary_monitor()
                                   out primary_monitor);
      x = primary_monitor.x;
      y = primary_monitor.y;
      width = primary_monitor.width;
      height = primary_monitor.height;

      /* The drag-n-drop region */
      drag_dest.resize (QUICKLAUNCHER_WIDTH,
                        height - PANEL_HEIGHT);
      drag_dest.move (x, y + PANEL_HEIGHT);

      /* We're responsible for painting the backgrounds on all the monitors */
      refresh_n_backgrounds (screen.get_n_monitors ());
      for (int i = 0; i < screen.get_n_monitors (); i++)
        {
          var bg = backgrounds.get (i);
          if (bg is Background)
            {
              Gdk.Rectangle rect;
              screen.get_monitor_geometry (i, out rect);

              bg.set_position (rect.x, rect.y);
              bg.set_size (rect.width, rect.height);
            }
        }   

      this.launcher.get_container ().set_size (this.QUICKLAUNCHER_WIDTH,
                                   (height-this.PANEL_HEIGHT));
      this.launcher.get_container ().set_position (x, y + this.PANEL_HEIGHT);
      this.launcher.get_container ().set_clip (0, 0,
                                   this.QUICKLAUNCHER_WIDTH,
                                   height-this.PANEL_HEIGHT);

      Utils.set_strut ((Gtk.Window)this.drag_dest,
                       this.QUICKLAUNCHER_WIDTH, y, (uint32)height,
                       PANEL_HEIGHT, x, (uint32)width);

      this.places.set_size (width - this.QUICKLAUNCHER_WIDTH, height);
      this.places.set_position (x + this.QUICKLAUNCHER_WIDTH, y);

      this.panel.set_size (width, PANEL_HEIGHT);
      this.panel.set_position (x, y);

      ensure_input_region ();

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
    public Clutter.Stage get_stage ()
    {
      return this.stage;
    }

    public void close_xids (Array<uint32> xids)
    {
      for (int i = 0; i < xids.length; i++)
        {
          uint32 xid = xids.index (i);

          unowned GLib.List<Mutter.Window> mutter_windows = this.plugin.get_windows ();
          foreach (Mutter.Window window in mutter_windows)
            {
              uint32 wxid = (uint32) Mutter.MetaWindow.get_xwindow (window.get_meta_window ());
              if (wxid == xid)
                {
                  Mutter.MetaWindow.delete (window.get_meta_window (), Clutter.get_current_event_time ());
                }
            }
        }
    }

    public void expose_xids (Array<uint32> xids)
    {
      spaces_manager.hide_spaces_picker ();
      SList<Clutter.Actor> windows = new SList<Clutter.Actor> ();
      for (int i = 0; i < xids.length; i++)
        {
          uint32 xid = xids.index (i);

          unowned GLib.List<Mutter.Window> mutter_windows = plugin.get_windows ();
          foreach (Mutter.Window w in mutter_windows)
            {
              uint32 wxid = (uint32) Mutter.MetaWindow.get_xwindow (w.get_meta_window ());
              if (wxid == xid)
                {
                  windows.append (w);
                  break;
                }
            }
        }

      expose_windows (windows,  get_launcher_width_foobar () + 10);
    }

    public void stop_expose ()
    {
      expose_manager.end_expose ();
    }

    public void show_window (uint32 xid)
    {
      unowned GLib.List<Mutter.Window> mutter_windows = this.plugin.get_windows ();

      foreach (Mutter.Window mutter_window in mutter_windows)
        {
          ulong window_xid = (ulong) Mutter.MetaWindow.get_xwindow (mutter_window.get_meta_window ());
          if (window_xid != xid)
            continue;

          int type = mutter_window.get_window_type ();

          if (type != Mutter.MetaWindowType.NORMAL &&
              type != Mutter.MetaWindowType.DIALOG &&
              type != Mutter.MetaWindowType.MODAL_DIALOG)
            continue;

          uint32 time_;
          unowned Mutter.MetaWindow meta = mutter_window.get_meta_window ();

          time_ = Mutter.MetaDisplay.get_current_time (Mutter.MetaWindow.get_display (meta));
          Mutter.MetaWorkspace.activate (Mutter.MetaWindow.get_workspace (meta), time_);
          Mutter.MetaWindow.activate (meta, time_);
        }
    }

    public ShellMode get_mode ()
    {
      return places_showing ? ShellMode.DASH : ShellMode.MINIMIZED;
    }

    public int get_indicators_width ()
    {
      return this.panel.get_indicators_width ();
    }

    public void expose_windows (GLib.SList<Clutter.Actor> windows,
                                int left_buffer = 75)
    {
      expose_manager.left_buffer = left_buffer;
      expose_manager.start_expose (windows);
    }


    public void hide_unity ()
    {
      if (places_showing == false)
        return;

      places_showing = false;

      var anim = dark_box.animate (Clutter.AnimationMode.EASE_IN_QUAD, 100, "opacity", 0);
      anim.completed.connect ((an) => {
        (an.get_object () as Clutter.Actor).destroy ();
      });

      plugin.get_normal_window_group ().animate (Clutter.AnimationMode.EASE_OUT_QUAD, 100, "opacity", 255);

      anim = places.animate (Clutter.AnimationMode.EASE_OUT_QUAD, 100,
                      "opacity", 0);
      anim.completed.connect ((an) => {
        (an.get_object () as Clutter.Actor).hide ();
        });

      panel.set_indicator_mode (false);
      ensure_input_region ();

      while (Gtk.events_pending ())
        Gtk.main_iteration ();

      places.hidden ();

      mode_changed (ShellMode.MINIMIZED);
    }

    public void show_unity ()
    {
      if (this.places_showing)
        {
          hide_unity ();
        }
      else
        {
          this.places_showing = true;

          this.places.show ();
          this.places.opacity = 0;
          this.dark_box = new Clutter.Rectangle.with_color ({0, 0, 0, 255});

          (this.plugin.get_window_group () as Clutter.Container).add_actor (this.dark_box);
          this.dark_box.raise (plugin.get_normal_window_group ());

          this.dark_box.set_position (primary_monitor.x, primary_monitor.y);
          this.dark_box.set_size (primary_monitor.width, primary_monitor.height);

          this.dark_box.show ();

          this.panel.set_indicator_mode (true);

          this.ensure_input_region ();

          new X.Display (null).flush ();

          this.dark_box.opacity = 0;

          this.dark_box.animate   (Clutter.AnimationMode.EASE_IN_QUAD, 100, "opacity", 180);
          plugin.get_normal_window_group ().animate (Clutter.AnimationMode.EASE_OUT_QUAD, 100, "opacity", 0);
          places.animate (Clutter.AnimationMode.EASE_OUT_QUAD, 100,
                          "opacity", 255);

          places.shown ();

          mode_changed (ShellMode.DASH);
        }
    }

    public void about_to_show_places ()
    {
      places.about_to_show ();
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

    private unowned Mutter.MetaWindow? get_window_for_xid (uint32 xid)
    {
      unowned GLib.List<Mutter.Window> mutter_windows = this.plugin.get_windows ();
      foreach (Mutter.Window window in mutter_windows)
        {
          uint32 wxid = (uint32) Mutter.MetaWindow.get_xwindow (window.get_meta_window ());
          if (wxid == xid)
            {
              unowned Mutter.MetaWindow win = window.get_meta_window ();
              return win;
            }
        }

      return null;
    }

    public void get_window_details (uint32   xid,
                                    out bool allows_resize,
                                    out bool is_maximised)
    {
      unowned Mutter.MetaWindow? win = get_window_for_xid (xid);

      if (win != null)
        {
          allows_resize = Mutter.MetaWindow.allows_resize (win);
          is_maximised = (Mutter.MetaWindow.is_maximized (win) ||
                          Mutter.MetaWindow.is_maximized_horizontally (win) ||
                          Mutter.MetaWindow.is_maximized_vertically (win));

        }
    }

    public void do_window_action (uint32 xid, WindowAction action)
    {
      unowned Mutter.MetaWindow? win = get_window_for_xid (xid);

      if (win != null)
        {
          switch (action)
            {
            case WindowAction.CLOSE:
              Mutter.MetaWindow.delete (win, get_current_time ());
              break;

            case WindowAction.MINIMIZE:
              Mutter.MetaWindow.minimize (win);
              break;

            case WindowAction.MAXIMIZE:
              Mutter.MetaWindow.maximize (win,
                                          Mutter.MetaMaximizeFlags.HORIZONTAL |
                                          Mutter.MetaMaximizeFlags.VERTICAL);
              break;

            case WindowAction.UNMAXIMIZE:
              Mutter.MetaWindow.unmaximize (win,
                                            Mutter.MetaMaximizeFlags.HORIZONTAL |
                                            Mutter.MetaMaximizeFlags.VERTICAL);
              break;

            default:
              warning (@"Window action type $action not supported");
              break;
            }
        }
    }

    private void on_gesture_received (Gesture.Event event)
    {
      if (active_gesture_type != Gesture.Type.NONE
          && active_gesture_type != event.type
          && event.state != Gesture.State.ENDED)
        {
          /* A new gesture is beginning */
          if (event.state == Gesture.State.BEGAN)
            {
              active_gesture_type = event.type;
            }
          else
            {
              /* We don't want to handle it */
              return;
            }
        }

      if (event.type == Gesture.Type.TAP)
        {
          if (event.fingers == 4
              && !expose_manager.expose_showing)
            {
              if (places_showing == true)
                hide_unity ();
              else
                show_unity ();
            }
              /*
              if (expose_manager.expose_showing == false)
                {
                }
              else
                expose_manager.end_expose ();
              */
        }
      else if (event.type == Gesture.Type.PINCH)
        {
          if (event.fingers == 3
              && places_showing == false)
            {
              if (event.state == Gesture.State.ENDED)
                {
                  if (event.pinch_event.radius_delta < 0)
                    {
                      /* Pinch */
                      if (expose_type == ExposeType.NONE || !expose_manager.expose_showing)
                        {
                          Mutter.Window? window = null;

                          var actor = stage.get_actor_at_pos (Clutter.PickMode.ALL,
                                                              (int)event.root_x,
                                                              (int)event.root_y);
                          if (actor is Mutter.Window == false)
                            actor = actor.get_parent ();

                          if (actor is Mutter.Window)
                            {
                              window = actor as Mutter.Window;

                              if (window.get_window_type () != Mutter.MetaCompWindowType.NORMAL &&
                                  window.get_window_type () != Mutter.MetaCompWindowType.DIALOG &&
                                  window.get_window_type () != Mutter.MetaCompWindowType.MODAL_DIALOG &&
                                  window.get_window_type () != Mutter.MetaCompWindowType.UTILITY)
                                window = null;
                            }

                          if (window is Mutter.Window)
                            {
                              var matcher = Bamf.Matcher.get_default ();
                              var xwin = (uint32)Mutter.MetaWindow.get_xwindow (window.get_meta_window ());

                              foreach (Bamf.Application app in matcher.get_running_applications ())
                                {
                                  Array<uint32> xids = app.get_xids ();
                                  for (int i = 0; i < xids.length; i++)
                                    {
                                      uint32 xid = xids.index (i);
                                      if (xwin == xid)
                                        {
                                          // Found the right application, so pick it
                                          expose_xids (xids);
                                          expose_type = ExposeType.APPLICATION;  
                                          return;
                                        }
                                    }
                                }
                            }

                          /* If we're here we didnt find window, so lets do window expose */
                          SList<Clutter.Actor> windows = new SList<Clutter.Actor> ();
                          unowned GLib.List<Mutter.Window> mutter_windows = plugin.get_windows ();
                          foreach (Mutter.Window w in mutter_windows)
                            {
                              windows.append (w);
                            }
                          expose_windows (windows,  get_launcher_width_foobar () + 10);
                          
                          expose_type = ExposeType.WINDOWS;

                        }
                      else if (expose_type == ExposeType.APPLICATION)
                        {
                          SList<Clutter.Actor> windows = new SList<Clutter.Actor> ();
                          unowned GLib.List<Mutter.Window> mutter_windows = plugin.get_windows ();
                          foreach (Mutter.Window w in mutter_windows)
                            {
                              windows.append (w);
                            }
                          expose_windows (windows,  get_launcher_width_foobar () + 10);
                          
                          expose_type = ExposeType.WINDOWS;
                        }
                      else if (expose_type == ExposeType.WINDOWS)
                        {
                          expose_type = ExposeType.WORKSPACE;
                        }
                    }
                  else
                    {
                      /* Spread */
                      if (expose_type == ExposeType.NONE || !expose_manager.expose_showing)
                        {
                          expose_manager.end_expose ();
                          expose_type = ExposeType.NONE;
                        }
                      else if (expose_type == ExposeType.APPLICATION)
                        {
                          expose_type = ExposeType.NONE;
                        }
                      else if (expose_type == ExposeType.WINDOWS)
                        {
                          expose_type = ExposeType.APPLICATION;
                        }
                      else if (expose_type == ExposeType.WORKSPACE)
                        {
                          expose_type = ExposeType.WINDOWS;
                        }
                    }
                }
            }
        }
      else if (event.type == Gesture.Type.PAN)
        {
          if (resize_window is Mutter.Window)
            return;

          if (event.fingers == 3)
            {
              if (event.state == Gesture.State.BEGAN
                  && event.pan_event.current_n_fingers == 2)
                {
                  start_pan_window = null;

                  var actor = stage.get_actor_at_pos (Clutter.PickMode.ALL,
                                                      (int)event.root_x,
                                                      (int)event.root_y);
                  if (actor is Mutter.Window == false)
                    actor = actor.get_parent ();

                  if (actor is Mutter.Window)
                    {
                      start_pan_window = actor as Mutter.Window;

                      if (start_pan_window.get_window_type () != Mutter.MetaCompWindowType.NORMAL &&
                          start_pan_window.get_window_type () != Mutter.MetaCompWindowType.DIALOG &&
                          start_pan_window.get_window_type () != Mutter.MetaCompWindowType.MODAL_DIALOG &&
                          start_pan_window.get_window_type () != Mutter.MetaCompWindowType.UTILITY)
                        start_pan_window = null;
                    }
                }
              else if (event.state == Gesture.State.CONTINUED
                       && event.pan_event.current_n_fingers == 2)
                {
                  if (start_pan_window is Mutter.Window == false)
                    return;

                  last_pan_x_root = event.root_x;

                  unowned Mutter.MetaWindow win = start_pan_window.get_meta_window ();
                  bool fullscreen = false;
                  win.get ("fullscreen", out fullscreen);

                  if (!Mutter.MetaWindow.is_maximized (win) && fullscreen == false)
                    {
                      if (start_pan_window.y == PANEL_HEIGHT
                          && event.pan_event.delta_y <= 0.0f)
                        {
                          if (start_frame_rect is Clutter.Rectangle == false)
                            {
                              Clutter.Rectangle frame = new Clutter.Rectangle.with_color ({ 0, 0, 0, 10 });
                              frame.border_color = { 255, 255, 255, 255 };
                              frame.border_width = 3;

                              stage.add_actor (frame);
                              frame.set_size (start_pan_window.width,
                                              start_pan_window.height);
                              frame.set_position (start_pan_window.x,
                                                  start_pan_window.y);
                              frame.show ();

                              start_frame_rect = frame;

                              last_pan_maximised_x_root = start_pan_window.x;
                            }

                          maximize_type = MaximizeType.FULL;
                          var MAX_DELTA = 50.0f;
                          if (start_pan_window.x < last_pan_maximised_x_root - MAX_DELTA)
                            {
                              maximize_type = MaximizeType.LEFT;
                              start_frame_rect.animate (Clutter.AnimationMode.EASE_OUT_QUAD,
                                                        150,
                                                        "x", (float)QUICKLAUNCHER_WIDTH,
                                                        "y", (float)PANEL_HEIGHT,
                                                        "width", (stage.width - QUICKLAUNCHER_WIDTH)/2.0f,
                                                        "height", stage.height - PANEL_HEIGHT);

                            }
                          else if (start_pan_window.x > last_pan_maximised_x_root + MAX_DELTA)
                            {
                              maximize_type = MaximizeType.RIGHT;
                              start_frame_rect.animate (Clutter.AnimationMode.EASE_OUT_QUAD,
                                                        150,
                                                        "x", (float)((stage.width - QUICKLAUNCHER_WIDTH)/2.0f) + QUICKLAUNCHER_WIDTH,
                                                        "y", (float)PANEL_HEIGHT,
                                                        "width", (stage.width - QUICKLAUNCHER_WIDTH)/2.0f,
                                                        "height", stage.height - PANEL_HEIGHT);
                            }
                          else
                            {
                              start_frame_rect.animate (Clutter.AnimationMode.EASE_OUT_QUAD,
                                                        150,
                                                        "x", (float)QUICKLAUNCHER_WIDTH,
                                                        "y", (float)PANEL_HEIGHT,
                                                        "width", stage.width - QUICKLAUNCHER_WIDTH,
                                                        "height", stage.height - PANEL_HEIGHT);

                             }
                        }
                     else
                        {
                          if (start_frame_rect is Clutter.Rectangle)
                            {
                              if (start_frame_rect.opacity == 0)
                                {
                                  stage.remove_actor (start_frame_rect);
                                  start_frame_rect = null;
                                }
                              else if (start_pan_window.y > PANEL_HEIGHT + 5 &&
                                (start_frame_rect.get_animation () is Clutter.Animation == false))
                                {
                                  start_frame_rect.animate (Clutter.AnimationMode.EASE_IN_QUAD,
                                                            150,
                                                            "x", start_pan_window.x,
                                                            "y", start_pan_window.y,
                                                            "width", start_pan_window.width,
                                                            "height", start_pan_window.height,
                                                            "opacity", 0);
                                }
                            }
                        }
                      start_pan_window.x += Math.floorf (event.pan_event.delta_x + 0.5f);
                      start_pan_window.y += Math.floorf (event.pan_event.delta_y + 0.5f);
                      start_pan_window.x = float.max (start_pan_window.x, QUICKLAUNCHER_WIDTH);
                      start_pan_window.y = float.max (start_pan_window.y, PANEL_HEIGHT);
                    }
                  else
                    {
                      if (event.pan_event.delta_y >= 0.0f && fullscreen == false)
                        {
                          Mutter.MetaWindow.unmaximize (win,
                                                        Mutter.MetaMaximizeFlags.HORIZONTAL | Mutter.MetaMaximizeFlags.VERTICAL);
                        }
                    }
                }
              else if (event.state == Gesture.State.ENDED)
                {
                  if (start_pan_window is Mutter.Window)
                    {
                      unowned Mutter.MetaWindow win = start_pan_window.get_meta_window ();
                      float nx = 0;
                      float ny = 0;
                      float nwidth = 0;
                      float nheight = 0;
                      bool  move_resize = false;
                      bool fullscreen = false;
                      win.get ("fullscreen", out fullscreen);

                      int wx, wy, ww, wh;
                      Mutter.MetaWindow.get_geometry (win, out wx, out wy, out ww, out wh);
                      Mutter.MetaRectangle rect = Mutter.MetaRectangle ();
                      Mutter.MetaWindow.get_outer_rect (win, rect);

                      if (Mutter.MetaWindow.is_maximized (win) || fullscreen)
                        {
                        }
                      else if (start_pan_window.y == PANEL_HEIGHT && event.pan_event.delta_y < 0.0f)
                        {
                          if (maximize_type == MaximizeType.FULL)
                            {
                            
                              Mutter.MetaWindow.maximize (win,
                                                          Mutter.MetaMaximizeFlags.HORIZONTAL | Mutter.MetaMaximizeFlags.VERTICAL);
                              move_resize = false;
                            }
                          else if (maximize_type == MaximizeType.RIGHT)
                            {
                              nx = (float)QUICKLAUNCHER_WIDTH + (stage.width-QUICKLAUNCHER_WIDTH)/2.0f;
                              ny = (float)PANEL_HEIGHT;
                              nwidth = (stage.width - QUICKLAUNCHER_WIDTH)/2.0f;
                              nheight = stage.height - PANEL_HEIGHT;

                              move_resize = true;
                            }
                          else if (maximize_type == MaximizeType.LEFT)
                            {
                              nx = (float)QUICKLAUNCHER_WIDTH;
                              ny = (float)PANEL_HEIGHT;
                              nwidth = (stage.width - QUICKLAUNCHER_WIDTH)/2.0f;
                              nheight = stage.height - PANEL_HEIGHT;

                              move_resize = true;
                            }
                        }
                      else
                        {
                          nx = start_pan_window.x;
                          ny = start_pan_window.y;
                          nwidth = 0.0f;
                          nheight = 0.0f;
                          move_resize = true;
                        }

                      if (move_resize)
                        {
                          X.Window xwin = start_pan_window.get_x_window ();
                          if (nwidth > 0.0f && nheight > 0.0f)
                            {
                              Mutter.MetaWindow.move_resize (win, false, ((int)nx),
                                                             ((int)ny),
                                                             (int)nwidth, (int)nheight);
                            }
                          else
                            {
                              /* We use gdk_window_move because we don't want
                               * to send in the width and height if we dont
                               * need to, as otherwise we'll cause a resize
                               * for no reason, and most likely get it
                               * wrong (you need to take into account frame
                               * size inside Mutter
                               */
                              unowned Gdk.Window w = Gdk.Window.foreign_new ((Gdk.NativeWindow)xwin);
                              w.move ((int)nx, (int)ny);
                            }
                        }
                    }

                    if (start_frame_rect is Clutter.Rectangle)
                      stage.remove_actor (start_frame_rect);
                }
            }
        }
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
      if (window.get_data<string> (UNDECORATED_HINT) == null)
        {
          Utils.window_set_decorations (Mutter.MetaWindow.get_xwindow (window.get_meta_window ()), 0);
        }

      this.window_maximized (this, window, x, y, width, height);

      active_window_state_changed ();
    }

    public void unmaximize (Mutter.Window window,
                            int           x,
                            int           y,
                            int           width,
                            int           height)
    {
      if (window.get_data<string> (UNDECORATED_HINT) == null)
        {
          Utils.window_set_decorations (Mutter.MetaWindow.get_xwindow (window.get_meta_window ()), 1);
        }

      this.window_unmaximized (this, window, x, y, width, height);

      active_window_state_changed ();
    }

    public void map (Mutter.Window window)
    {
      unowned Mutter.MetaWindow win = window.get_meta_window ();

      if (window.get_window_type () == Mutter.MetaCompWindowType.NORMAL)
        {
          Idle.add (() => {
            if (win is Object)
              {
                bool decorated  = Utils.window_is_decorated (Mutter.MetaWindow.get_xwindow (win));
                bool maximized  = Mutter.MetaWindow.is_maximized (win);
                
                bool fullscreen;
                win.get ("fullscreen", out fullscreen);
                
                if (!decorated && !maximized)
                  {
                    window.set_data (UNDECORATED_HINT, "%s".printf ("true"));
                  }
                else if (decorated && maximized && !fullscreen)
                  {
                    Utils.window_set_decorations (Mutter.MetaWindow.get_xwindow (win), 0);
                  }
              }

            return false;
          });
        }
      else if (window.get_window_type () == Mutter.MetaCompWindowType.DOCK)
        {
          if (win.get_xwindow (win) == Gdk.x11_drawable_get_xid (drag_dest.window))
            {
              window.opacity = 0;
            }
        }

      this.maximus.process_window (window);
      this.window_mapped (this, window);

      if (display == null)
        {
          display = Mutter.MetaWindow.get_display (window.get_meta_window ());
          display.notify["focus-window"].connect (on_focus_window_changed);
        }
    }

    public void destroy (Mutter.Window window)
    {
      this.window_destroyed (this, window);
    }

    public void switch_workspace (int                 from,
                                  int                 to,
                                  int                 direction)
    {
      this.workspace_switch_event (this, from, to, direction);
    }

    public void on_kill_window_effects (Mutter.Window window)
    {
      this.kill_window_effects (this, window);
    }

    public void on_kill_switch_workspace ()
    {
      this.kill_switch_workspace (this);
    }

    public int get_panel_height ()
    {
      return this.PANEL_HEIGHT;
    }

    public int get_launcher_width ()
    {
      return this.QUICKLAUNCHER_WIDTH;
    }

    /* this is needed to avoid a symbol clash in unity/targets/mutter/main.c */
    public int get_panel_height_foobar ()
    {
      return this.get_panel_height ();
    }

    /* this is needed to avoid a symbol clash in unity/targets/mutter/main.c */
    public int get_launcher_width_foobar ()
    {
      return this.get_launcher_width ();
    }
  }
}
