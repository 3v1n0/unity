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
      this.plugin.kill_window_effects.connect (this.kill_window_effects);
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

    private void window_maximized (Plugin        plugin,
                                   Mutter.Window window,
                                   int           x,
                                   int           y,
                                   int           width,
                                   int           height)
    {
      /*
       * If the user purposly maximised a window, we want to unset any hint
       * that we put on it asking maximus to avoid the window
       */
      window.set_data (Maximus.user_unmaximize_hint, null);

      plugin.plugin.maximize_completed (window);
    }

    private void window_unmaximized (Plugin        plugin,
                                     Mutter.Window window,
                                     int           x,
                                     int           y,
                                     int           width,
                                     int           height)
    {
      /* If the user has purposly unmaxmised a window, we want to let the
       * maximus plugin know of this, so it doesn't try and maximize it when
       * the user wants to unminimise the window
       */

      int i = 1;
      window.set_data (Maximus.user_unmaximize_hint, i.to_pointer ());

      plugin.plugin.unmaximize_completed (window);
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
          this.plugin.plugin.minimize_completed (window);
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
      unowned Mutter.Window window = anim.get_object () as Mutter.Window;

      if (window == null)
        return;

      window.hide ();
      this.plugin.plugin.minimize_completed (window);
    }

    private bool force_activate ()
    {
      if (this.last_mapped is Mutter.Window)
        {
          unowned Mutter.MetaWindow w = this.last_mapped.get_meta_window ();
          unowned Mutter.MetaDisplay d = Mutter.MetaWindow.get_display (w);

          Mutter.MetaWindow.activate (this.last_mapped.get_meta_window (),
                                      Mutter.MetaDisplay.get_current_time (d));
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
          this.plugin.plugin.map_completed (window);
          return;
        }

      if (type == Mutter.MetaWindowType.NORMAL ||
          type == Mutter.MetaWindowType.DIALOG)
        {
          Mutter.MetaWindow.activate (window.get_meta_window (),
                                      Mutter.MetaWindow.get_user_time (
                                            window.get_meta_window ()));
          this.last_mapped = window;
          Idle.add (this.force_activate);
        }

      Clutter.Animation anim = null;
      Clutter.Actor actor = window as Clutter.Actor;
      actor.opacity = 0;
      window.show ();

      int speed = get_animation_speed (window);

      Mutter.MetaRectangle rect = {0, 0, 0, 0};
      if (Mutter.MetaWindow.get_icon_geometry (window.get_meta_window (), rect))
        {
          rect = {0, 0, 0, 0};
          Mutter.MetaWindow.get_outer_rect (window.get_meta_window (), rect);
          actor.set ("scale-gravity", Clutter.Gravity.CENTER);
          anim = actor.animate (Clutter.AnimationMode.EASE_IN_SINE, speed,
                         "opacity", 255,
                         "x", (float) rect.x,
                         "y", (float) rect.y,
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
      unowned Mutter.Window window = anim.get_object () as Mutter.Window;

      if (window == null)
        return;

      (window as Clutter.Actor).opacity = 255;
      this.plugin.plugin.map_completed (window);
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
          this.plugin.plugin.destroy_completed (window);
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
      unowned Mutter.Window window = (Mutter.Window)anim.get_object ();

      this.plugin.plugin.destroy_completed (window);
    }

    private void kill_window_effects (Plugin        plugin,
                                      Mutter.Window window)
    {
     /* var anim = window.get_animation ();

      if (anim is Clutter.Animation)
        anim.completed ();
        */
    }
  }
}
