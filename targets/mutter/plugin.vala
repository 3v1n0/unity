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

    public ExposeManager expose_manager { get; private set; }
    public Background    background     { get; private set; }

    public bool menus_swallow_events { get { return false; } }

    public bool expose_showing { get { return expose_manager.expose_showing; } }

    private static const int PANEL_HEIGHT        =  24;
    private static const int QUICKLAUNCHER_WIDTH = 58;
    private static const string UNDECORATED_HINT = "UNDECORATED_HINT";

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
    private float start_pinch_radius = 0.0f;
    private unowned Mutter.Window? start_pan_window = null;
    private unowned Clutter.Rectangle? start_frame_rect = null;
    private float last_pan_x_root = 0.0f;

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
          this.screensaver.ActiveChanged.connect (got_screensaver_changed);
        }
      catch (Error e)
        {
          warning (e.message);
        }

      this.wm = new WindowManagement (this);
      this.maximus = new Maximus ();

      END_FUNCTION ();
    }

    private bool real_construct ()
    {
      START_FUNCTION ();

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

      Clutter.Group window_group = (Clutter.Group) this.plugin.get_window_group ();

      this.background = new Background ();
      this.stage.add_actor (background);
      this.background.lower_bottom ();
      this.background.show ();

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

      gesture_dispatcher = new Gesture.GeisDispatcher ();
      gesture_dispatcher.gesture.connect (on_gesture_received);
      
      this.ensure_input_region ();
      return false;
      
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

      this.launcher.get_container ().set_size (this.QUICKLAUNCHER_WIDTH,
                                   (height-this.PANEL_HEIGHT));
      this.launcher.get_container ().set_position (0, this.PANEL_HEIGHT);
      this.launcher.get_container ().set_clip (0, 0,
                                   this.QUICKLAUNCHER_WIDTH,
                                   height-this.PANEL_HEIGHT);
      Utils.set_strut ((Gtk.Window)this.drag_dest,
                       this.QUICKLAUNCHER_WIDTH, 0, (uint32)height,
                       PANEL_HEIGHT, 0, (uint32)width);

      this.places.set_size (width - this.QUICKLAUNCHER_WIDTH, height);
      this.places.set_position (this.QUICKLAUNCHER_WIDTH, 0);

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

          this.dark_box.set_position (0, 0);
          this.dark_box.set_size (this.stage.width, this.stage.height);

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
      if (event.type == Gesture.Type.TAP &&
          expose_manager.expose_showing == false)
        {
          if (event.fingers == 3)
            {
              debug ("Move Window");
              print (@"$event");
            }
          else if (event.fingers == 4)
            {
              show_unity ();
            }
        }
      else if (event.type == Gesture.Type.PINCH &&
               places_showing == false)
        {
          if (event.fingers == 3)
            {
              debug ("Resize Window");
            }
          else if (event.fingers == 4)
            {
              if (event.state == Gesture.State.BEGAN)
                {
                  if (expose_manager.expose_showing)
                    {
                      expose_manager.end_expose ();
                    }
                  else
                    {
                      SList<Clutter.Actor> windows = new SList<Clutter.Actor> ();
                      unowned GLib.List<Mutter.Window> mutter_windows = plugin.get_windows ();
                      foreach (Mutter.Window w in mutter_windows)
                        {
                          windows.append (w);
                        }
                      expose_windows (windows,  get_launcher_width_foobar () + 10);
                    }

                  foreach (ExposeClone clone in expose_manager.exposed_windows)
                    {
                      clone.get_animation ().get_timeline ().pause ();
                    }

                  start_pinch_radius = event.pinch_event.radius_delta;
                }
              else if (event.state == Gesture.State.CONTINUED)
                {
                  foreach (ExposeClone clone in expose_manager.exposed_windows)
                    {
                      int I_JUST_PULLED_THIS_FROM_MY_FOO = 5;

                      if (event.pinch_event.radius_delta >= 0)
                        {
                          if (start_pinch_radius < 0)
                            {
                              /* We're moving backward */
                              I_JUST_PULLED_THIS_FROM_MY_FOO *= -1;
                            }
                        }
                      else
                        {
                          if (start_pinch_radius >= 0)
                            {
                              /* We're moving backward */
                              I_JUST_PULLED_THIS_FROM_MY_FOO *= -1;
                            }
                        }

                      var tl = clone.get_animation ().get_timeline ();
                      tl.advance (tl.get_elapsed_time ()
                                  + I_JUST_PULLED_THIS_FROM_MY_FOO);
                     
                      float factor = (float)tl.get_elapsed_time () /
                                     (float)tl.get_duration ();
                      
                      Value v = Value (typeof (float));
                      
                      var interval = clone.get_animation ().get_interval ("x");
                      interval.compute_value (factor, v);
                      clone.x = v.get_float ();

                      interval = clone.get_animation ().get_interval ("y");
                      interval.compute_value (factor, v);
                      clone.y = v.get_float ();

                      double scalex, scaley;
                      clone.get_scale (out scalex, out scaley);

                      v = Value (typeof (double));

                      interval = clone.get_animation ().get_interval ("scale-x");
                      interval.compute_value (factor, v);
                      scalex = v.get_double ();

                      interval = clone.get_animation ().get_interval ("scale-y");
                      interval.compute_value (factor, v);
                      scaley = v.get_double ();

                      clone.set_scale (scalex, scaley);
                    }
                 }
              else if (event.state == Gesture.State.ENDED)
                {
                  foreach (ExposeClone clone in expose_manager.exposed_windows)
                    {
                      var anim = clone.get_animation ();
                      
                      Value v = Value (typeof (float));
                      
                      var interval = anim.get_interval ("x");
                      v.set_float (clone.x);
                      interval.set_initial_value (v);

                      interval = anim.get_interval ("y");
                      v.set_float (clone.y);
                      interval.set_initial_value (v);

                      v = Value (typeof (double));
                      double scalex, scaley;
                      clone.get_scale (out scalex, out scaley);

                      interval = anim.get_interval ("scale-x");
                      v.set_double (scalex);
                      interval.set_initial_value (v);

                      interval = anim.get_interval ("scale-y");
                      v.set_double (scaley);
                      interval.set_initial_value (v);

                      clone.get_animation ().get_timeline ().start ();
                      clone.queue_relayout ();
                    }
                }
            }
        }
      else if (event.type == Gesture.Type.PAN)
        {
          if (event.fingers == 3)
            {
              if (event.state == Gesture.State.BEGAN)
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
              else if (event.state == Gesture.State.CONTINUED)
                {
                  if (start_pan_window is Mutter.Window == false)
                    return;

                  last_pan_x_root = event.root_x;

                  unowned Mutter.MetaWindow win = start_pan_window.get_meta_window ();
                  if (!Mutter.MetaWindow.is_maximized (win))
                    {
                      if (start_pan_window.y == PANEL_HEIGHT
                          && event.pan_event.delta_y < 0.0f)
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

                              frame.animate (Clutter.AnimationMode.EASE_OUT_QUAD,
                                             150,
                                             "x", (float)QUICKLAUNCHER_WIDTH,
                                             "y", (float)PANEL_HEIGHT,
                                             "width", stage.width - QUICKLAUNCHER_WIDTH,
                                             "height", stage.height - PANEL_HEIGHT);

                              start_frame_rect = frame;
                            }
                        }
                     else
                        {
                          start_pan_window.x += Math.floorf (event.pan_event.delta_x);
                          start_pan_window.y += Math.floorf (event.pan_event.delta_y);
                          start_pan_window.x = float.max (start_pan_window.x, QUICKLAUNCHER_WIDTH);
                          start_pan_window.y = float.max (start_pan_window.y, PANEL_HEIGHT);


                              //if ((start_pan_window.x + start_pan_window.width) >= stage.width && (start_pan_window.x + start_pan_window.width) <= (stage.width + 5.0))
                          if (event.root_x > stage.width - 160) /* FIXME: Need the box */
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

                                  frame.animate (Clutter.AnimationMode.EASE_OUT_QUAD,
                                                 150,
                                                 "x", (float)((stage.width - QUICKLAUNCHER_WIDTH)/2.0f) + QUICKLAUNCHER_WIDTH,
                                                 "y", (float)PANEL_HEIGHT,
                                                 "width", (stage.width - QUICKLAUNCHER_WIDTH)/2.0f,
                                                 "height", stage.height - PANEL_HEIGHT);

                                  start_frame_rect = frame;
                                }
                            }
                          else if (event.root_x < QUICKLAUNCHER_WIDTH + 160) /* FIXME: Need the box */
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

                                  frame.animate (Clutter.AnimationMode.EASE_OUT_QUAD,
                                                 150,
                                                 "x", (float)QUICKLAUNCHER_WIDTH,
                                                 "y", (float)PANEL_HEIGHT,
                                                 "width", (stage.width - QUICKLAUNCHER_WIDTH)/2.0f,
                                                 "height", stage.height - PANEL_HEIGHT);

                                  start_frame_rect = frame;
                                }
                            }
                          else if (start_frame_rect is Clutter.Rectangle)
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
                    }
                  else
                    {
                      if (event.pan_event.delta_y >= 0.0f)
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

                      print (@"$event\n");

                      if (start_pan_window.y == PANEL_HEIGHT)
                        {
                          if (event.pan_event.delta_y < 0.0f)
                            Mutter.MetaWindow.maximize (win,
                                                        Mutter.MetaMaximizeFlags.HORIZONTAL | Mutter.MetaMaximizeFlags.VERTICAL);

                        }
                      else if (last_pan_x_root >= stage.width - 160)
                        {
                          nx = (float)QUICKLAUNCHER_WIDTH + (stage.width-QUICKLAUNCHER_WIDTH)/2.0f;
                          ny = (float)PANEL_HEIGHT;
                          nwidth = (stage.width - QUICKLAUNCHER_WIDTH)/2.0f;
                          nheight = stage.height - PANEL_HEIGHT;

                          move_resize = true;

                          debug ("RIGH RESIZE : %f %f %f %f", nx, ny, nwidth, nheight);
                        }
                      else if (last_pan_x_root <= QUICKLAUNCHER_WIDTH + 160)
                        {
                          nx = (float)QUICKLAUNCHER_WIDTH;
                          ny = (float)PANEL_HEIGHT;
                          nwidth = (stage.width - QUICKLAUNCHER_WIDTH)/2.0f;
                          nheight = stage.height - PANEL_HEIGHT;

                          move_resize = true;

                          debug ("LEFT RESIZE : %f %f %f %f", nx, ny, nwidth, nheight);
                        }
                      else
                        {
                          nx = start_pan_window.x;
                          ny = start_pan_window.y;
                          nwidth = start_pan_window.width;
                          nheight = start_pan_window.height;

                          move_resize = true;
                          debug ("MOVE RESIZE : %f %f %f %f", nx, ny, nwidth, nheight); 
                        }

                      //debug ("REQUEST: %f %f %f %f", x, y, width, height);

                      X.Window xwin = start_pan_window.get_x_window ();
                      unowned Gdk.Window w = Gdk.Window.foreign_new ((Gdk.NativeWindow)xwin);
                      if (w is Gdk.Window && move_resize)
                        {
                          /*
                             Gdk.error_trap_push ();
 
                          if (move_resize && width > 0.0f && height > 0.0f)
                            {
                              //w.resize ((int)width, (int)height);
                              //start_pan_window.width = width;
                              //start_pan_window.height = height;
                            }
                          if (move_resize)
                            {
                              //w.move ((int)x, (int)y);
                              //start_pan_window.x = x;
                              //start_pan_window.y = y;
                            }
*/
                          Mutter.MetaWindow.move_resize (win, false, (int)nx, (int)ny, (int)nwidth, (int)nheight);
                          debug ("RESIZE : %f %f %f %f", nx, ny, nwidth, nheight);
                          //Gdk.flush ();
                          //Gdk.error_trap_pop ();
                        }
                    }

                    if (start_frame_rect is Clutter.Rectangle)
                      stage.remove_actor (start_frame_rect);
                    
                    //print (@"$event\n");
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
      /*FIXME: This doesn't work in Mutter
      if (window.get_data<string> (UNDECORATED_HINT) == null)
        {
          uint32 xid = (uint32)window.get_x_window ();
          Utils.window_set_decorations (xid, 0);
        }
        */

      this.window_maximized (this, window, x, y, width, height);

      active_window_state_changed ();
    }

    public void unmaximize (Mutter.Window window,
                            int           x,
                            int           y,
                            int           width,
                            int           height)
    {
      /* FIXME: This doesn't work in Mutter
      if (window.get_data<string> (UNDECORATED_HINT) == null)
        {
          uint32 xid = (uint32)window.get_x_window ();
          Utils.window_set_decorations (xid, 1);
        }
      */

      this.window_unmaximized (this, window, x, y, width, height);

      active_window_state_changed ();
    }

    public void map (Mutter.Window window)
    {
      /* FIXME: This doesn't work in Mutter
      uint32 xid = (uint32)window.get_x_window ();
      if (Utils.window_is_decorated (xid) == false)
        window.set_data (UNDECORATED_HINT, "%s".printf ("true"));
      */
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
