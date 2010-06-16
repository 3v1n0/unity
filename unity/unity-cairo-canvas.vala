/*
 * Copyright (C) 2010 Canonical, Ltd.
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License
 * version 3.0 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License version 3.0 for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Authored by Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */

namespace Unity
{
  /*
   * CairoCanvas handles the ClutterCairoTexture size-allocation and updating
   * but delegates the drawing to an external source. This allows you to set
   * a CairoCanvas as the background of a CtkActor, but keep the drawing inside
   * the same class as the CtkActor.
   * As CairoCanvas is a CtkBin, this also means you can add an CtkEffect to it.
   */
  public class CairoCanvas : Ctk.Bin
  {
    public delegate void CairoCanvasPaint (Cairo.Context cr,
                                           int           width,
                                           int           height);

    public Clutter.CairoTexture texture;

    private int last_width = 0;
    private int last_height = 0;

    private CairoCanvasPaint paint_func;

    public CairoCanvas (CairoCanvasPaint _func)
    {
      Object ();

      paint_func = _func;
    }

    construct
    {
      texture = new Clutter.CairoTexture (10, 10);
      add_actor (texture);
      texture.show ();
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

      if (last_width < 1 || last_height < 1)
        return false;

      texture.set_surface_size (last_width, last_height);

      cr = texture.create ();

      paint_func (cr, last_width, last_height);

      unowned GLib.SList<unowned Ctk.Effect> effects = get_effects ();
      foreach (unowned Ctk.Effect effect in effects)
        effect.set_invalidate_effect_cache (true);

      return false;
    }

    public void update ()
    {
      update_background ();
    }
  }
}
