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

    public WindowButtons ()
    {
      Object (orientation:Ctk.Orientation.HORIZONTAL,
              spacing:4,
              homogeneous:false);
    }

    construct
    {
    }

    private override void get_preferred_width (float     for_height,
                                               out float min_width,
                                               out float nat_width)
    {
      min_width = 70.0f;
      nat_width = min_width;
    }
  }

  public class WindowButton : Ctk.Button
  {
    public static const string AMBIANCE = "/usr/share/themes/Ambiance/metacity-1";

    public string filename { get; construct; }

    public WindowButton (string filename)
    {
      Object (filename:filename);
    }

    construct
    {
      try {

        var bg = new Clutter.Texture.from_file (AMBIANCE + "/" + filename + ".png");
        set_background_for_state (Ctk.ActorState.STATE_NORMAL, bg);

      } catch (Error e) {
        warning (@"Unable to load window button theme: You need Ambiance installed: $(e.message)");
      }
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
