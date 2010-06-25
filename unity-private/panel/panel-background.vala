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
  public class Background : Clutter.CairoTexture
  {
    public const string BG = "/usr/share/unity/themes/panel_background.png";
    private int last_width = 0;
    private int last_height = 0;

    Gdk.Pixbuf tile = null;

    public Background ()
    {
      Object ();
    }

    construct
    {
      try
        {
          /* I've loaded this in directly and not through theme due to use
           * not having a good loader for pixbufs from the theme right now
           */
          tile = new Gdk.Pixbuf.from_file (BG);
        }
      catch (Error e)
        {
          warning ("Unable to load panel background");
        }
    }

    private override void allocate (Clutter.ActorBox        box,
                                    Clutter.AllocationFlags flags)
    {
      int width = (int)(box.x2 - box.x1);
      int height = (int)(box.y2 - box.y1);

      base.allocate (box, flags);

      if (width != last_width || height != last_height)
        {
          last_width = width;
          last_height = height;

          Timeout.add (0, update_background);
        }
    }

    private bool update_background ()
    {
      Cairo.Context cr;

      set_surface_size (last_width, last_height);

      cr = create ();

      cr.set_operator (Cairo.Operator.CLEAR);
      cr.paint ();

      cr.set_operator (Cairo.Operator.OVER);
      cr.set_line_width (1.0);

      cr.rectangle (0, 0, last_width, last_height);
      cr.set_source_rgb (61/255.0, 60/255.0, 56/255.0);
      cr.fill ();

      cr.rectangle (0, 0, last_width, last_height-3);
      if (tile is Gdk.Pixbuf)
        {
          Gdk.cairo_set_source_pixbuf (cr, tile, 0, 0);
          var pat = cr.get_source ();
          pat.set_extend (Cairo.Extend.REPEAT);
          cr.fill ();
        }

      return false;
    }
  }
}
