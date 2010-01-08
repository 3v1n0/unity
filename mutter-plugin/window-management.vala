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

    private void window_minimized (Plugin plugin, Mutter.Window window)
    {
      plugin.plugin.effect_completed (window, Mutter.PLUGIN_MINIMIZE);
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

    private void window_mapped (Plugin plugin, Mutter.Window window)
    {
      /* We only want some types of windows */
      int type = window.get_window_type ();
      if (type == Mutter.MetaCompWindowType.NORMAL)
        {
          window.opacity = 0;
          window.show ();

          //Mutter.MetaWindow.maximize (window.get_meta_window (), Mutter.MetaMaximizeFlags.HORIZONTAL);
          //Mutter.MetaWindow.maximize (window.get_meta_window (), Mutter.MetaMaximizeFlags.VERTICAL);

          var anim = window.animate (Clutter.AnimationMode.EASE_IN_SINE, 300,
                                     "opacity", 255);

          anim.completed.connect (this.window_mapped_completed);
        }
      else
        {
          this.plugin.plugin.effect_completed (window, Mutter.PLUGIN_MAP);
        }
    }
    
    private void window_mapped_completed (Clutter.Animation anim)
    {
      Mutter.Window window = (Mutter.Window)anim.get_object ();

      window.opacity = 255;
      this.plugin.plugin.effect_completed (window, Mutter.PLUGIN_MAP);
    }
    
    private void window_destroyed (Plugin plugin, Mutter.Window window)
    {
      /* We only want some types of windows */
      int type = window.get_window_type ();
      if (type != Mutter.MetaCompWindowType.NORMAL
          && type != Mutter.MetaCompWindowType.DIALOG
          && type != Mutter.MetaCompWindowType.MODAL_DIALOG)
        {
         plugin.plugin.effect_completed (window, Mutter.PLUGIN_DESTROY);
         return;
        }
      
      var anim = window.animate (Clutter.AnimationMode.EASE_IN_SINE, 200, 
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
