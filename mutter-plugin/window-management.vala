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
    
    public WindowManagement (Plugin p)
    {
      this.plugin = p;
      this.plugin.window_minimized.connect (this.window_minimized);
      this.plugin.window_maximized.connect (this.window_maximized);
      this.plugin.window_unmaximized.connect (this.window_unmaximized);
      this.plugin.window_mapped.connect (this.window_mapped);
      this.plugin.window_destroyed.connect (this.window_destroyed);
      this.plugin.window_kill_effect.connect (this.window_kill_effect);
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
      (window as Clutter.Actor).get_animation ().send_completed ();
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
      (window as Clutter.Actor).get_animation ().send_completed ();
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
                         "x-scale", scale,
                         "y-scale", scale);
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
      window.opacity = 0;
      this.plugin.plugin.effect_completed (window, Mutter.PLUGIN_MINIMIZE);
    }

    private void window_mapped (Plugin plugin, Mutter.Window window)
    {
      (window as Clutter.Actor).get_animation ().send_completed ();
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
                         "x-scale", 1f,
                         "y-scale", 1f);
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

      window.opacity = 255;
      this.plugin.plugin.effect_completed (window, Mutter.PLUGIN_MAP);
    }
    
    private void window_destroyed (Plugin plugin, Mutter.Window window)
    {
      (window as Clutter.Actor).get_animation ().send_completed ();
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
