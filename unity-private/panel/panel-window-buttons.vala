/*
 * Copyright (C) 2010 Canonical Ltd
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

namespace Unity.Panel
{
  public class WindowButtons : Ctk.Box
  {
    private WindowButton close;
    private WindowButton minimise;
    private WindowButton maximise;

    private unowned Bamf.Matcher matcher;

    public WindowButtons ()
    {
      Object (orientation:Ctk.Orientation.HORIZONTAL,
              spacing:2,
              homogeneous:false);
    }

    construct
    {
      close = new WindowButton ("close.png");
      pack (close, false, false);
      close.show ();

      minimise = new WindowButton ("minimize.png");
      pack (minimise, false, false);
      minimise.show ();

      maximise = new WindowButton ("maximize.png");
      pack (maximise, false, false);
      maximise.show ();

      matcher = Bamf.Matcher.get_default ();
      matcher.active_window_changed.connect (on_active_window_changed);
    }

    private void on_active_window_changed (GLib.Object? object,
                                           GLib.Object? object1)
    {
      Bamf.View? old_view = object as Bamf.View;
      Bamf.View? new_view = object1 as Bamf.View;

      if (new_view is Bamf.Window)
        {
          string name = new_view.get_name ();
          debug ("Active window changed: %s", name);
        }
    }

    private override void get_preferred_width (float     for_height,
                                               out float min_width,
                                               out float nat_width)
    {
      min_width = 72.0f;
      nat_width = min_width;
    }
  }

  public class WindowButton : Ctk.Button
  {
    public static const string AMBIANCE = "/usr/share/themes/Ambiance/metacity-1";

    public string filename { get; construct; }
    public Clutter.Actor bg;

    public WindowButton (string filename)
    {
      Object (filename:filename);
    }

    construct
    {
      try {

        bg = new Ctk.Image.from_filename (20, AMBIANCE + "/" + filename);
        add_actor (bg);
        bg.show ();

      } catch (Error e) {
        warning (@"Unable to load window button theme: You need Ambiance installed: $(e.message)");
      }

      notify["state"].connect (() => {
        switch (state)
          {
          case Ctk.ActorState.STATE_NORMAL:
            bg.opacity = 255;
            break;

          case Ctk.ActorState.STATE_PRELIGHT:
            bg.opacity = 120;
            break;

          case Ctk.ActorState.STATE_ACTIVE:
          default:
            bg.opacity = 50;
            break;
          }
      });
    }

    private override void get_preferred_width (float     for_height,
                                               out float min_width,
                                               out float nat_width)
    {
      min_width = 20.0f;
      nat_width = min_width;
    }

    private override void get_preferred_height (float     for_width,
                                                out float min_height,
                                                out float nat_height)
    {
      min_height = 18.0f;
      nat_height = min_height;
    }
  }
}
