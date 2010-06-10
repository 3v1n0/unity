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
  public class IndicatorBackground : Clutter.CairoTexture
  {
    public const string BG = "/usr/share/unity/themes/panel_background.png";
    private int last_width = 0;
    private int last_height = 0;
    
    Gdk.Pixbuf tile = null;

    public IndicatorBackground ()
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
      int width = (int)(box.x2 - box.x1) + 12;
      int height = (int)(box.y2 - box.y1);

      Clutter.ActorBox child;
      child = box;
      child.x1 = child.x1 - 12;
      child.x2 = child.x2 + 12;
      
      base.allocate (child, flags);

      if (width != last_width || height != last_height)
        {
          last_width = width;
          last_height = height;

          Idle.add (update_background);
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

      //cr.translate (0.5f, 0.5f);
      
      cr.move_to (-1, -1);
      cr.line_to (last_width+1, -1);
      cr.line_to (last_width+1, last_height);
      cr.line_to (12,         last_height);
      cr.arc (12.0, last_height - 6.0f, 6, -Math.PI/2.0, 1.5*Math.PI/2.0);
      cr.line_to (-1,          -1);

      cr.set_source_rgb (61/255.0, 60/255.0, 56/255.0);
      cr.fill ();

      
      cr.move_to (-1, -1);
      cr.line_to (last_width+1, -1);
      cr.line_to (last_width+1, last_height-3);
      cr.line_to (12,         last_height-3);
      cr.arc (12.0, last_height - 9.0f, 6, -Math.PI/2.0, 1.5*Math.PI/2.0);
      cr.line_to (-1,          -1);
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
