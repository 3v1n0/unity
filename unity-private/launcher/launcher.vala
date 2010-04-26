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

  public interface ShortcutItem : GLib.Object
  {
    public abstract string get_name ();

    public abstract void activated ();
  }

  public class Launcher : Object
  {
    public Shell shell {get; construct;}
    private ScrollerController controller;
    private ScrollerView view;
    private ScrollerModel model;

    public Launcher (Shell shell)
    {
      Object (shell: shell);
    }

    construct
    {
      model = new ScrollerModel ();
      view = new ScrollerView (model);
      controller = new ScrollerController (model, view);
    }

    public new float get_width ()
    {
      return 60;
    }

    public Clutter.Actor get_view ()
    {
      return view as Clutter.Actor;
    }
  }
}
