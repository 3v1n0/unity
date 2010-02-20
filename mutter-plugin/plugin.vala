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

    public bool expose_showing { get; private set; }

    private static const int PANEL_HEIGHT        = 23;
    private static const int QUICKLAUNCHER_WIDTH = 60;

    private Clutter.Stage    stage;
    private Application      app;
    private WindowManagement wm;
    private Maximus          maximus;

    /* Unity Components */
    private Background         background;
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

    private List<Clutter.Actor> exposed_windows;

    private bool grab_enabled = false;

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

      /* Unique instancing */
      LOGGER_START_PROCESS ("unity_application_constructor");
      this.app = new Unity.Application ();
      this.app.shell = this;
      LOGGER_END_PROCESS ("unity_application_constructor");
      END_FUNCTION ();
    }

    private void real_construct ()
    {
      START_FUNCTION ();
      this.wm = new WindowManagement (this);
      this.maximus = new Maximus ();

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

      this.quicklauncher = new Quicklauncher.View (this);
      this.quicklauncher.opacity = 0;

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

      Wnck.Screen.get_default().active_window_changed.connect (on_active_window_changed);

      if (Wnck.Screen.get_default ().get_active_window () != null)
        Wnck.Screen.get_default ().get_active_window ().state_changed.connect (on_active_window_state_changed);
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


    }

    private void on_launcher_menu_opened (LauncherView sender)
    {
      if (sender != quicklauncher.manager.active_launcher || sender == null)
        return;

      if (sender.model is ApplicationModel && sender.model.is_active)
        {
          expose_windows ((sender.model as ApplicationModel).windows);
        }
    }

    private void on_launcher_menu_closed (LauncherView sender)
    {
      if (sender != quicklauncher.manager.active_launcher)
        return;

      dexpose_windows ();
    }

    void check_fullscreen_obstruction ()
    {
      Wnck.Window current = Wnck.Screen.get_default ().get_active_window ();
      if (current == null)
        return;

      if (current.is_fullscreen ())
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
                       this.QUICKLAUNCHER_WIDTH, 0, (uint32)height,
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

    private InputState last_input_state = InputState.NONE;
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
      else if (expose_showing ||
               places_showing ||
               Unity.Quicklauncher.active_menu != null ||
               Unity.Drag.Controller.get_default ().is_dragging
              )
        {
          // Fullscreen required
          if (last_input_state == InputState.FULLSCREEN)
            return;

          last_input_state = InputState.FULLSCREEN;
          this.restore_input_region (true);

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
      if (this.expose_showing == true)
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

      this.expose_windows (windows, 40);
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
      exposed_windows = new List<Mutter.Window> ();

      unowned GLib.List<Mutter.Window> mutter_windows = this.plugin.get_windows ();
      foreach (Mutter.Window w in mutter_windows)
        {
          bool keep = false;
          ulong xid = (ulong) Mutter.MetaWindow.get_xwindow (w.get_meta_window ());
          foreach (Wnck.Window window in windows)
            {
              if (window.get_xid () == xid)
                {
                  keep = true;
                  break;
                }
            }

          if (keep)
            {
              Clutter.Actor clone = new Clutter.Clone (w);
              clone.set_position (w.x, w.y);
              clone.set_size (w.width, w.height);
              clone.opacity = w.opacity;
              exposed_windows.append (clone);
              clone.reactive = true;

              unowned Clutter.Container container = w.get_parent () as Clutter.Container;

              container.add_actor (clone);
              clone.raise (w);
            }

            if (w.get_window_type () == Mutter.MetaCompWindowType.DESKTOP)
              {
                exposed_windows.reverse ();
                foreach (Clutter.Actor actor in exposed_windows)
                  {
                    actor.raise (w);
                  }
                exposed_windows.reverse ();
                continue;
              }
            (w as Clutter.Actor).reactive = false;
            (w as Clutter.Actor).animate (Clutter.AnimationMode.EASE_IN_SINE, 80, "opacity", 0);
        }

      position_window_on_grid (exposed_windows, left_buffer);

      expose_showing = true;
      this.ensure_input_region ();
      this.stage.captured_event.connect (on_stage_captured_event);
    }

    private void dexpose_windows ()
    {
      if (quicklauncher.manager.active_launcher != null)
        quicklauncher.manager.active_launcher.close_menu ();

      unowned GLib.List<Mutter.Window> mutter_windows = this.plugin.get_windows ();
      foreach (Mutter.Window window in mutter_windows)
        {
          window.opacity = 255;
          window.reactive = true;
        }

      foreach (Clutter.Actor actor in exposed_windows)
        restore_window_position (actor);

      this.expose_showing = false;
      this.ensure_input_region ();
    }

    private void restore_window_position (Clutter.Actor actor)
    {
      if (!(actor is Clutter.Clone))
        return;

      Clutter.Actor window = (actor as Clutter.Clone).source;

      uint8 opacity = window.opacity;
      actor.set ("scale-gravity", Clutter.Gravity.CENTER);
      Clutter.Animation anim = actor.animate (Clutter.AnimationMode.EASE_IN_OUT_SINE, 250,
                                         "scale-x", 1f,
                                         "scale-y", 1f,
                                         "opacity", opacity,
                                         "x", window.x,
                                         "y", window.y);

      window.opacity = 0;

      anim.completed.connect (() => {
        actor.destroy ();
        window.opacity = 255;
      });
    }

    void position_window_on_grid (List<Clutter.Actor> _windows,
                                  int left_buffer = 250)
    {
      List<Clutter.Actor> windows = _windows.copy ();
      int vertical_buffer = 40;
      int count = (int) windows.length ();

      int cols = (int) Math.ceil (Math.sqrt (count));

      int rows = 1;

      while (cols * rows < count)
        rows++;

      int boxWidth = (int) ((this.stage.width - left_buffer) / cols);
      int boxHeight = (int) ((this.stage.height - vertical_buffer * 2) / rows);

      for (int row = 0; row < rows; row++)
        {
          for (int col = 0; col < cols; col++)
            {
              if (windows.length () == 0)
                return;

              int centerX = (boxWidth / 2 + boxWidth * col + left_buffer);
              int centerY = (boxHeight / 2 + boxHeight * row + vertical_buffer);

              Clutter.Actor window = null;
              // greedy layout
              foreach (Clutter.Actor actor in windows)
                {
                  if (window == null)
                    {
                      window = actor;
                      continue;
                    }

                  double window_distance = Math.sqrt (
                    Math.fabs (centerX - (window.x + window.width / 2)) +
                    Math.fabs (centerY - (window.y + window.height / 2))
                  );
                  double actor_distance = Math.sqrt (
                    Math.fabs (centerX - (actor.x + actor.width / 2)) +
                    Math.fabs (centerY - (actor.y + actor.height / 2))
                  );

                  if (actor_distance < window_distance)
                    window = actor;
                }

              windows.remove (window);

              int windowX = centerX - (int) window.width / 2;
              int windowY = centerY - (int) window.height / 2;

              float scale = float.min (float.min (1, (boxWidth - 20) / window.width), float.min (1, (boxHeight - 20) / window.height));

              window.set ("scale-gravity", Clutter.Gravity.CENTER);
              window.animate (Clutter.AnimationMode.EASE_IN_OUT_SINE, 250,
                                                 "x", (float) windowX,
                                                 "y", (float) windowY,
                                                 "opacity", 255,
                                                 "scale-x", scale,
                                                 "scale-y", scale);
            }
        }
    }

    bool on_stage_captured_event (Clutter.Event event)
    {
      if (event.type == Clutter.EventType.ENTER || event.type == Clutter.EventType.LEAVE)
        return false;

      bool event_over_menu = false;

      float x, y;
      event.get_coords (out x, out y);

      unowned Clutter.Actor actor = this.stage.get_actor_at_pos (Clutter.PickMode.REACTIVE, (int) x, (int) y);

      unowned Clutter.Actor menu = null;
      if (Unity.Quicklauncher.active_menu != null)
        menu = Unity.Quicklauncher.active_menu.menu as Clutter.Actor;
      if (menu != null)
        {
          if (x > menu.x && x < menu.x + menu.width && y > menu.y && y < menu.y + menu.height)
            event_over_menu = true;
        }

      if (event.type == Clutter.EventType.BUTTON_RELEASE && event.get_button () == 1)
        {
          while (actor.get_parent () != null && !(actor is Clutter.Clone))
            actor = actor.get_parent ();

          Clutter.Clone clone = actor as Clutter.Clone;
          if (clone != null && clone.source is Mutter.Window)
            {
              clone.raise_top ();
              unowned Mutter.MetaWindow meta = ((actor as Clutter.Clone).source as Mutter.Window).get_meta_window ();
              Mutter.MetaWorkspace.activate (Mutter.MetaWindow.get_workspace (meta), event.get_time ());
              Mutter.MetaWindow.activate (meta, event.get_time ());
            }
          this.stage.captured_event.disconnect (on_stage_captured_event);
          this.dexpose_windows ();
        }

      return !event_over_menu;
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
