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

namespace Unity.Launcher
{
  const uint SHORT_DELAY = 400;
  const uint MEDIUM_DELAY = 800;
  const uint LONG_DELAY = 1600;

  public class LauncherContainer : Ctk.Bin
  {
    public Ctk.EffectCache cache;
    public LauncherContainer ()
    {
      this.cache = new Ctk.EffectCache ();
      this.add_effect (this.cache);

      this.cache.update_texture_cache ();
    }
  }
  
  public interface ShortcutItem : GLib.Object
  {
    public abstract string get_name ();

    public abstract void activated ();
  }

  public class Launcher : Object
  {
    public Shell shell {get; construct;}
    public ScrollerModel model { get; private set; }

    private ScrollerController controller;
    private ScrollerView view;

    private LauncherContainer launcher_container;

    public Launcher (Shell shell)
    {
      Object (shell: shell);
    }

    construct
    {
      model = new ScrollerModel ();
      
      this.launcher_container = new LauncherContainer ();
      view = new ScrollerView (model, this.launcher_container.cache);
      
      controller = new ScrollerController (model, view);

      view.queue_redraw.connect (() => {
        if (view.fling_timeline.is_playing () == false)
          launcher_container.cache.update_texture_cache ();
      });
    }

    public new float get_width ()
    {
      return 60;
    }

    public Clutter.Actor get_view ()
    {
      return view as Clutter.Actor;
    }
    
    public Clutter.Actor get_container ()
    {
      return this.launcher_container as Clutter.Actor;
    }

    public Ctk.EffectCache get_actor_cache ()
    {
      return this.launcher_container.cache;
    }

  }
}
