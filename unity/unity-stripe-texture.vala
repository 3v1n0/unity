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
   * UnityStripeTexture will paint itself in the stripe-style that's used
   * in many places in Unity. This is best-used as a background for CtkActor.
   * However there are some options to make the painting of more complex
   * objects easier
   */
  public class StripeTexture : CairoCanvas
  {
    /* Set the outline_paint_func if you want to draw your own outline for
     * the stripe to paint in
     */
    public delegate void StripeTextureOutlineFunc (Cairo.Context cr,
                                                   int           width,
                                                   int           height);

    public float radius { get; construct set; }

    public StripeTextureOutlineFunc outline_paint_func;

    private Cairo.Surface? pattern;

    public StripeTexture (StripeTextureOutlineFunc? func)
    {
      Object (radius:10.0f);

      outline_paint_func = rounded_outline;
      if (func != null)
        outline_paint_func = func;
    }

    construct
    {
      paint_func = paint_bg;
    }

    public void rounded_outline (Cairo.Context cr, int width, int height)
    {
      var x = 0;
      var y = 0;
      width -= 1;
      height -= 1;

      cr.line_to  (x, y + radius);
      cr.curve_to (x, y,
                   x, y,
                   x + radius, y);
      cr.line_to  (width - radius, y);
      cr.curve_to (width, y,
                   width, y,
                   width, y + radius);
      cr.line_to  (width, height - radius);
      cr.curve_to (width, height,
                   width, height,
                   width - radius, height);
      cr.line_to  (x + radius, height);
      cr.curve_to (x, height,
                   x, height,
                   x, height - radius);
      cr.close_path ();
    }

    private void paint_bg (Cairo.Context cr, int width, int height)
    {
      cr.set_operator (Cairo.Operator.CLEAR);
      cr.paint ();

      cr.set_operator (Cairo.Operator.OVER);
      cr.set_line_width (1.0);

      cr.translate (0.5, 0.5);

      outline_paint_func (cr, width, height);

      if (pattern == null)
        {
          pattern = new Cairo.Surface.similar (cr.get_target (),
                                               Cairo.Content.COLOR_ALPHA,
                                               4, 4);
          var context = new Cairo.Context (pattern);
          
          context.set_operator (Cairo.Operator.CLEAR);
          context.paint ();

          context.set_line_width (0.3);
          context.set_operator (Cairo.Operator.OVER);
          context.set_source_rgba (1.0, 1.0, 1.0, 0.65);

          context.move_to (0, 0);
          context.line_to (4, 4);

          context.stroke ();
        }

      var pat = new Cairo.Pattern.for_surface (pattern);
      pat.set_extend (Cairo.Extend.REPEAT);
      cr.set_source (pat);
      cr.fill_preserve ();

      cr.set_line_width (1.75);
      cr.set_source_rgba (1.0, 1.0, 1.0, 0.5);
      cr.stroke ();
    }
  }
}
