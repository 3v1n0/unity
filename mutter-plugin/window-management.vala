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

namespace Unity
{
  public class WindowManagement : Object
  {
    private Plugin plugin;
    private List<Clutter.Actor> switch_windows;
    private Clutter.Group workgroup1;
    private Clutter.Group workgroup2;
    private int switch_signals_to_send;

    private unowned Mutter.Window last_mapped;

    public WindowManagement (Plugin p)
    {
      this.plugin = p;
      this.plugin.window_minimized.connect (this.window_minimized);
      this.plugin.window_maximized.connect (this.window_maximized);
      this.plugin.window_unmaximized.connect (this.window_unmaximized);
      this.plugin.window_mapped.connect (this.window_mapped);
      this.plugin.window_destroyed.connect (this.window_destroyed);
      this.plugin.window_kill_effect.connect (this.window_kill_effect);
      this.plugin.workspace_switch_event.connect (this.workspace_switched);
    }

    construct
    {
    }

    private int get_animation_speed (Mutter.Window window)
    {
      int type = window.get_window_type ();
      if (type == Mutter.MetaCompWindowType.NORMAL
          || type == Mutter.MetaCompWindowType.DIALOG
          || type == Mutter.MetaCompWindowType.MODAL_DIALOG)
        return 200;
      return 80;
    }

    private void workspace_switched (Plugin plugin,
                                     List<Mutter.Window> windows,
                                     int from,
                                     int to,
                                     int direction)
    {
      if (plugin.expose_showing)
        {
          Mutter.Window window = windows.nth_data (0);
          plugin.plugin.effect_completed (window, Mutter.PLUGIN_SWITCH_WORKSPACE);
          return;
        }

      switch_signals_to_send++;

      unowned Clutter.Actor stage = plugin.plugin.get_stage ();
      float x_delta = 0;
      float y_delta = 0;
      Clutter.Animation anim = null;

      if (direction == -4)
        x_delta = stage.width;
      else if (direction == -3)
        x_delta = -stage.width;
      else if (direction == -2)
        y_delta = stage.height;
      else if (direction == -1)
        y_delta = -stage.height;

      if (switch_signals_to_send > 1)
        {
          workgroup2.get_animation ().completed.disconnect (on_workspace_switch_completed);
          on_workspace_switch_completed (null);
        }

      switch_windows = new List<Clutter.Actor> ();

      workgroup1 = new Clutter.Group ();
      workgroup2 = new Clutter.Group ();

      (plugin.plugin.get_window_group () as Clutter.Container).add_actor (workgroup1);
      (plugin.plugin.get_window_group () as Clutter.Container).add_actor (workgroup2);

      (workgroup1 as Clutter.Actor).raise (plugin.plugin.get_normal_window_group ());
      (workgroup2 as Clutter.Actor).raise (plugin.plugin.get_normal_window_group ());


      foreach (Mutter.Window window in windows)
        {

          Clutter.Actor clone = new Clutter.Clone (window);
          switch_windows.prepend (clone);

          clone.set_position (window.x, window.y);
          clone.set_size (window.width, window.height);

          window.opacity = 0;
          clone.opacity = 255;

          if (window.get_window_type () == Mutter.MetaCompWindowType.DESKTOP)
          {
            (plugin.plugin.get_window_group () as Clutter.Container).add_actor (clone);
            clone.raise (plugin.plugin.get_normal_window_group ());
            continue;
          }


          if (window.get_workspace () == from)
            {
              workgroup1.add_actor (clone);
            }
          else if (window.get_workspace () == to)
            {
              workgroup2.add_actor (clone);
            }
        }

      anim = workgroup1.animate (Clutter.AnimationMode.LINEAR, 150,
                                 "x", -x_delta,
                                 "y", -y_delta);

      float y = workgroup2.y;
      float x = workgroup2.x;

      workgroup2.x = x_delta;
      workgroup2.y = y_delta;

      anim = workgroup2.animate (Clutter.AnimationMode.LINEAR, 150,
                            "x", x,
                            "y", y);
      anim.completed.connect (on_workspace_switch_completed);
    }

    private void on_workspace_switch_completed (Clutter.Animation? anim)
    {
      Mutter.Window window = null;

      foreach (Clutter.Actor actor in switch_windows)
        {
          window = (actor as Clutter.Clone).get_source () as Mutter.Window;
          window.opacity = 255;
          actor.destroy ();
        }

      workgroup1.destroy ();
      workgroup2.destroy ();

      plugin.plugin.effect_completed (window, Mutter.PLUGIN_SWITCH_WORKSPACE);
      switch_signals_to_send--;
    }

    private void window_maximized (Plugin        plugin,
                                   Mutter.Window window,
                                   int           x,
                                   int           y,
                                   int           width,
                                   int           height)
    {
      plugin.plugin.effect_completed (window, Mutter.PLUGIN_MAXIMIZE);
    }

    private void window_unmaximized (Plugin        plugin,
                                     Mutter.Window window,
                                     int           x,
                                     int           y,
                                     int           width,
                                     int           height)
    {
      plugin.plugin.effect_completed (window, Mutter.PLUGIN_UNMAXIMIZE);
    }

    private void window_minimized (Plugin plugin, Mutter.Window window)
    {
      int type = window.get_window_type ();

      if (type != Mutter.MetaWindowType.NORMAL &&
          type != Mutter.MetaWindowType.DIALOG &&
          type != Mutter.MetaWindowType.MODAL_DIALOG &&
          type != Mutter.MetaWindowType.MENU
          )
        {
          this.plugin.plugin.effect_completed (window, Mutter.PLUGIN_MINIMIZE);
          return;
        }

      Mutter.MetaRectangle rect = {0, 0, 0, 0};

      int speed = get_animation_speed (window);
      Clutter.Animation anim = null;
      Clutter.Actor actor = window as Clutter.Actor;
      if (Mutter.MetaWindow.get_icon_geometry (window.get_meta_window (), rect))
        {
          float scale = float.min (float.min (1, rect.width / actor.width), float.min (1, rect.height / actor.height));

          actor.set ("scale-gravity", Clutter.Gravity.CENTER);
          anim = actor.animate (Clutter.AnimationMode.EASE_IN_SINE, speed,
                         "opacity", 0,
                         "x", (float) ((rect.x + rect.width / 2) - (actor.width / 2)),
                         "y", (float) ((rect.y + rect.height / 2) - (actor.height / 2)),
                         "scale-x", scale,
                         "scale-y", scale);
        }
      else
        {
          anim = actor.animate (Clutter.AnimationMode.EASE_IN_SINE, speed, "opacity", 0);
        }

      anim.completed.connect (this.window_minimized_completed);
    }

    private void window_minimized_completed (Clutter.Animation anim)
    {
      Mutter.Window window = anim.get_object () as Mutter.Window;

      if (window == null)
        return;

      window.hide ();
      this.plugin.plugin.effect_completed (window, Mutter.PLUGIN_MINIMIZE);
    }

    private bool force_activate ()
    {
      if (this.last_mapped is Mutter.Window)
        {
          unowned Mutter.MetaWindow w = this.last_mapped.get_meta_window ();
          Mutter.MetaWindow.activate (this.last_mapped.get_meta_window (),
                                      Mutter.MetaWindow.get_user_time (w));
        }

      return false;
    }

    private void window_mapped (Plugin plugin, Mutter.Window window)
    {
      int type = window.get_window_type ();

      if (type != Mutter.MetaWindowType.NORMAL &&
          type != Mutter.MetaWindowType.DIALOG &&
          type != Mutter.MetaWindowType.MODAL_DIALOG &&
          type != Mutter.MetaWindowType.MENU
          )
        {
          this.plugin.plugin.effect_completed (window, Mutter.PLUGIN_MAP);
          return;
        }

      if (type == Mutter.MetaWindowType.NORMAL ||
          type == Mutter.MetaWindowType.DIALOG)
        {
          this.last_mapped = window;
          Idle.add (this.force_activate);
        }

      Clutter.Animation anim = null;
      Clutter.Actor actor = window as Clutter.Actor;
      actor.opacity = 0;
      window.show ();

      int speed = get_animation_speed (window);

      ulong xid = (ulong) Mutter.MetaWindow.get_xwindow (window.get_meta_window ());
      Wnck.Window wnck_window = Wnck.Window.get (xid);

      Mutter.MetaRectangle rect = {0, 0, 0, 0};
      if (wnck_window is Wnck.Window &&
          Mutter.MetaWindow.get_icon_geometry (window.get_meta_window (), rect))
        {
          int x, y, w, h;
          wnck_window.get_geometry (out x, out y, out w, out h);
          actor.set ("scale-gravity", Clutter.Gravity.CENTER);
          anim = actor.animate (Clutter.AnimationMode.EASE_IN_SINE, speed,
                         "opacity", 255,
                         "x", (float) x,
                         "y", (float) y,
                         "scale-x", 1f,
                         "scale-y", 1f);
        }
      else
        {
          anim = actor.animate (Clutter.AnimationMode.EASE_IN_SINE, speed, "opacity", 255);
        }

      anim.completed.connect (this.window_mapped_completed);
    }

    private void window_mapped_completed (Clutter.Animation anim)
    {
      Mutter.Window window = anim.get_object () as Mutter.Window;

      if (window == null)
        return;

      (window as Clutter.Actor).opacity = 255;
      this.plugin.plugin.effect_completed (window, Mutter.PLUGIN_MAP);
    }

    private void window_destroyed (Plugin plugin, Mutter.Window window)
    {
      int type = window.get_window_type ();

      if (type != Mutter.MetaWindowType.NORMAL &&
          type != Mutter.MetaWindowType.DIALOG &&
          type != Mutter.MetaWindowType.MODAL_DIALOG &&
          type != Mutter.MetaWindowType.MENU
          )
        {
          this.plugin.plugin.effect_completed (window, Mutter.PLUGIN_DESTROY);
          return;
        }

      Clutter.Animation anim = null;

      int speed = get_animation_speed (window);

      anim = window.animate (Clutter.AnimationMode.EASE_IN_SINE, speed,
                             "opacity", 0);

      anim.completed.connect (this.window_destroyed_completed);
    }

    private void window_destroyed_completed (Clutter.Animation anim)
    {
      Mutter.Window window = (Mutter.Window)anim.get_object ();

      this.plugin.plugin.effect_completed (window, Mutter.PLUGIN_DESTROY);
    }

    private void window_kill_effect (Plugin        plugin,
                                     Mutter.Window window,
                                     ulong         events)
    {
     /* var anim = window.get_animation ();

      if (anim is Clutter.Animation)
        anim.completed ();
        */
    }
  }
}
