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

namespace Unity.Places
{
  public class PlaceSearchEntry : Ctk.Box
  {
    static const string SEARCH_ICON_FILE = PKGDATADIR + "/search_icon.png";
    static const float  PADDING = 1.0f;
    static const int    LIVE_SEARCH_TIMEOUT = 300; /* Milliseconds */

    public Ctk.Image left_icon;
    public Ctk.Text  text;
    public ThemeImage right_icon;

    private uint live_search_timeout = 0;

    public signal void text_changed (string text);

    public PlaceSearchEntry ()
    {
      Object (orientation:Ctk.Orientation.HORIZONTAL,
              homogeneous:false,
              spacing:6);
    }

    construct
    {
      padding = { PADDING, PADDING * 4, PADDING , PADDING * 4};

      left_icon = new Ctk.Image.from_filename (18, SEARCH_ICON_FILE);
      pack (left_icon, false, true);
      left_icon.show ();

      text = new Ctk.Text ("Search Me...");
      text.reactive = true;
      text.selectable = true;
      text.editable = true;
      text.activatable = true;
      text.single_line_mode = true;
      pack (text, true, true);
      text.show ();

      text.text_changed.connect (on_text_changed);

      right_icon = new ThemeImage ("gtk-close");
      pack (right_icon, false, true);
      right_icon.show ();
    }

    private override void get_preferred_height (float     for_width,
                                               out float min_height,
                                               out float nat_height)
    {
      float mheight, nheight;
      text.get_preferred_height (400, out mheight, out nheight);

      min_height = mheight > 22.0f ? mheight : 22.0f;
      min_height += PADDING * 2;
      nat_height = min_height;
    }

    private void on_text_changed ()
    {
      if (live_search_timeout != 0)
        Source.remove (live_search_timeout);

      live_search_timeout = Timeout.add (LIVE_SEARCH_TIMEOUT, () => {
        text_changed (text.text);
        live_search_timeout = 0;

        return false;
      });
    }
  }
}

