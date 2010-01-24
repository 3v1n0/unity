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
    private static const int QUICKLAUNCHER_WIDTH = 58;

    private Clutter.Stage    stage;
    private Application      app;
    private WindowManagement wm;

    /* Unity Components */
    private Background         background;
    private Quicklauncher.View quicklauncher;
    private Places.Controller  places_controller;
    private Places.View        places;
    private Panel.View         panel;

    private DragDest drag_dest;
    private bool     places_showing;

    construct
    {
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

      this.places_controller = new Places.Controller (this);
      this.places = this.places_controller.get_view ();
      this.places.opacity = 0;
      this.stage.add_actor (this.places);
      this.stage.raise_child (this.places, window_group);
      this.places_showing = false;

      this.panel = new Panel.View (this);
      this.stage.add_actor (this.panel);
      this.stage.raise_child (this.panel, this.places);
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

      this.restore_input_region ();
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
      Utils.set_strut ((Gtk.Window)this.drag_dest, 58, 0, (uint32)height);

      this.places.set_size (width, height);
      this.places.set_position (0, 0);

      this.panel.set_size (width, 24);
      this.panel.set_position (0, 0);

      /* Leaving this here to remind me that we need to use these when
       * there are fullscreen windows etc
       * this.plugin.set_stage_input_region (uint region);
       * this.plugin.set_stage_reactive (true);
       */
      END_FUNCTION ();
    }

    private void on_stage_actor_added (Clutter.Actor actor)
    {
      if (actor is Ctk.Menu)
        {
          actor.destroy.connect (() =>
            {
              this.restore_input_region ();
            });
          this.plugin.set_stage_input_area (0, 0,
                                            (int)this.stage.width,
                                            (int)this.stage.height);
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

    public void show_unity ()
    {
      if (this.places_showing)
        {
          this.places_showing = false;
          this.places.animate (Clutter.AnimationMode.EASE_OUT_SINE, 300,
                               "opacity", 0);
          var win_group = this.plugin.get_window_group ();
          win_group.animate (Clutter.AnimationMode.EASE_IN_SINE, 300,
                             "opacity", 255);

          this.panel.set_indicator_mode (false);
          this.restore_input_region ();
        }
      else
        {
          this.places_showing = true;
          this.places.animate (Clutter.AnimationMode.EASE_IN_SINE, 300,
                               "opacity", 255);

          var win_group = this.plugin.get_window_group ();
          win_group.animate (Clutter.AnimationMode.EASE_OUT_SINE, 300,
                              "opacity", 0);

          this.panel.set_indicator_mode (true);

          this.plugin.set_stage_input_area (0,
                                            0,
                                            (int)this.stage.width,
                                            (int)this.stage.height);
        }
      debug ("Places showing: %s", this.places_showing ? "true":"false");
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
      return 24;
    }

    public int get_launcher_width ()
    {
      return 58;
    }
  }
}
