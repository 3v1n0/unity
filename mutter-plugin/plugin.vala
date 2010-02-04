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

    public signal void restore_input_region ();

    /* Properties */
    private Mutter.Plugin? _plugin;
    public  Mutter.Plugin? plugin {
      get { return _plugin; }
      set { _plugin = value; this.real_construct (); }
    }

    private static const int PANEL_HEIGHT        = 23;
    private static const int QUICKLAUNCHER_WIDTH = 60;

    private Clutter.Stage    stage;
    private Application      app;
    private WindowManagement wm;

    /* Unity Components */
    private Background         background;
    private Quicklauncher.View quicklauncher;
    private Places.Controller  places_controller;
    private Places.View        places;
    private Panel.View         panel;

    private bool     places_enabled;

    private DragDest drag_dest;
    private bool     places_showing;
    private bool     expose_showing;
    private List<Mutter.Window> exposed_windows;
    
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

      this.stage = (Clutter.Stage)this.plugin.get_stage ();
      this.stage.actor_added.connect (this.on_stage_actor_added);

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

      this.places_enabled = !envvar_is_enabled ("UNITY_DISABLE_PLACES");

      this.background = new Background ();
      this.stage.add_actor (this.background);
      this.background.lower_bottom ();

      Clutter.Group window_group = (Clutter.Group) this.plugin.get_window_group ();

      this.quicklauncher = new Quicklauncher.View (this);
      this.quicklauncher.opacity = 0;
      window_group.add_actor (this.quicklauncher);
      window_group.raise_child (this.quicklauncher,
                                this.plugin.get_normal_window_group ());
      this.quicklauncher.animate (Clutter.AnimationMode.EASE_IN_SINE, 400,
                                  "opacity", 255);

      if (this.places_enabled)
        {
          this.places_controller = new Places.Controller (this);
          this.places = this.places_controller.get_view ();

          this.places.opacity = 0;
          window_group.add_actor (this.places);
          window_group.raise_child (this.places,
                                    this.quicklauncher);
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

      this.on_restore_input_region ();
      
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
    
    void check_fullscreen_obstruction ()
    {
      Wnck.Window current = Wnck.Screen.get_default ().get_active_window ();
      if (current == null)
        return;
      
      if (current.is_fullscreen ())
        {
          this.quicklauncher.animate (Clutter.AnimationMode.EASE_IN_SINE, 200, "x", -100f);
          this.panel.animate (Clutter.AnimationMode.EASE_IN_SINE, 200, "opacity", 0);
          this.plugin.set_stage_input_area(0, 0, 0, 0);
        }
      else
        {
          this.quicklauncher.animate (Clutter.AnimationMode.EASE_IN_SINE, 200, "x", 0f);
          this.panel.animate (Clutter.AnimationMode.EASE_IN_SINE, 200, "opacity", 255);
          this.on_restore_input_region ();
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

    public void set_in_menu (bool is_in_menu)
    {
      /* we need to modify the stage input area so that the menus are
       * responsive and they detect clicks elsewhere
       */
      float width, height;
      this.stage.get_size (out width, out height);

      if (is_in_menu)
        {
          this.plugin.set_stage_input_area(0, 0, (int)(width), (int)(height));
          new X.Display (null).flush ();
        }
      else
        {
          this.on_restore_input_region ();
        }
    }
    
    void on_restore_input_region ()
    {
      if (!expose_showing)
        this.restore_input_region ();
    }

    private void on_stage_actor_added (Clutter.Actor actor)
    {
      if (actor is Ctk.Menu)
        {
          actor.destroy.connect (() =>
            {
              this.on_restore_input_region ();
            });

    this.plugin.set_stage_input_area (0, 0,
                                            (int)this.stage.width,
                                            (int)this.stage.height);
          new X.Display (null).flush ();
        }
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

    public int get_indicators_width ()
    {
      return this.panel.get_indicators_width ();
    }

    public void expose_windows (GLib.SList<Wnck.Window> windows)
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
              exposed_windows.append (w);
              (w as Clutter.Actor).reactive = true;
            }
          else
            {
              (w as Clutter.Actor).reactive = false;
              (w as Clutter.Actor).opacity = 0;
            }
        }
      position_window_on_grid (exposed_windows);
      
      expose_showing = true;
      this.stage.captured_event.connect (on_stage_captured_event);
    }
    
    public void dexpose_windows ()
    {
      unowned GLib.List<Mutter.Window> mutter_windows = this.plugin.get_windows ();
      foreach (Mutter.Window w in mutter_windows)
        restore_window_position (w);
    }
    
    void restore_window_position (Mutter.Window window)
    {
      (window as Clutter.Actor).animate (Clutter.AnimationMode.EASE_OUT_SINE, 250, "opacity", 255);
      (window as Clutter.Actor).reactive = true;
            
      if (exposed_windows.find (window) == null)
        return;
       
      ulong xid = (ulong) Mutter.MetaWindow.get_xwindow (window.get_meta_window ());
      Wnck.Window wnck_window = Wnck.Window.get (xid);
      
      int x, y, w, h;
      wnck_window.get_geometry (out x, out y, out w, out h);
      
      
      (window as Clutter.Actor).set ("scale-gravity", Clutter.Gravity.CENTER);
      (window as Clutter.Actor).animate (Clutter.AnimationMode.EASE_OUT_SINE, 250, 
                                         "scale-x", 1f, 
                                         "scale-y", 1f, 
                                         "x", (float) x, 
                                         "y", (float) y);
      
      this.expose_showing = false;
      this.on_restore_input_region ();
    }
    
    bool on_stage_captured_event (Clutter.Event event)
    {
      bool result = false;
      if (event.type == Clutter.EventType.BUTTON_RELEASE && event.get_button () == 1)
        {
          float x, y;
          event.get_coords (out x, out y);
          Clutter.Actor actor = this.stage.get_actor_at_pos (Clutter.PickMode.REACTIVE, (int) x, (int) y);
          
          if (actor is Mutter.Window)
            {
              Mutter.MetaWindow.activate ((actor as Mutter.Window).get_meta_window (), Gtk.get_current_event_time ());
              result = true;
            }
          this.stage.captured_event.disconnect (on_stage_captured_event);
          dexpose_windows ();
        }
      return result;
    }
    
    void position_window_on_grid (List<Mutter.Window> windows)
    {
      int left_buffer = 250;
      int vertical_buffer = 40;
      int count = (int) windows.length ();
      
      int rows = (int) Math.sqrt (count);
      if (rows == 0)
        rows = 1;
      
      int cols = rows;
      
      if (cols * rows < count)
        rows++;
      
      int boxWidth = (int) ((this.stage.width - left_buffer) / cols);
      int boxHeight = (int) ((this.stage.height - vertical_buffer * 2) / rows);
      
      for (int row = 0; row < rows; row++)
        {
          for (int col = 0; col < cols; col++)
            {
              int i = row * cols + col;
              if (i > windows.length ())
                return;
              
              Mutter.Window window = windows.nth_data (i);
              
              int windowX = (boxWidth / 2 + boxWidth * col + left_buffer) - (int) window.width / 2;
              int windowY = (boxHeight / 2 + boxHeight * row + vertical_buffer) - (int) window.height / 2;
              
              float scale = float.min (float.min (1, (boxWidth - 20) / window.width), float.min (1, (boxHeight - 20) / window.height));
              
              (window as Clutter.Actor).set ("scale-gravity", Clutter.Gravity.CENTER);
              
              (window as Clutter.Actor).animate (Clutter.AnimationMode.EASE_OUT_SINE, 250, 
                                                 "x", (float) windowX, 
                                                 "y", (float) windowY, 
                                                 "scale-x", scale, 
                                                 "scale-y", scale);
            }
        }
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
          this.plugin.get_above_window_group ().opacity = 255;
          this.plugin.get_normal_window_group ().opacity = 255;
          this.plugin.get_below_window_group().opacity = 255;

          this.panel.set_indicator_mode (false);

          this.on_restore_input_region ();
        }
      else
        {
          this.places_showing = true;

          this.places.show ();
          this.places.opacity = 255;
          this.plugin.get_above_window_group ().opacity = 0;
          this.plugin.get_normal_window_group ().opacity = 0;
          this.plugin.get_below_window_group().opacity = 0;

          this.panel.set_indicator_mode (true);

          this.plugin.set_stage_input_area (0,
                                            0,
                                            (int)this.stage.width,
                                            (int)this.stage.height);
          new X.Display (null).flush ();
        }
    }

    private bool envvar_is_enabled (string name)
    {
      if (Environment.get_variable (name) != null)
        return true;

      return false;
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
      debug ("Switch workplace");
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
