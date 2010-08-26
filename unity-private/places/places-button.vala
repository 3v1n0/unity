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
using Unity;

namespace Unity.Places
{
  public class Button : Ctk.Button
  {
    public delegate void ButtonOutlineFunc (Cairo.Context cr,
                                            int width,
                                            int height);
    private CairoCanvas     bg;
    private Ctk.EffectGlow? glow;

    public ButtonOutlineFunc outline_func;

    public Button ()
    {
      Object ();
    }

    construct
    {
      bg = new CairoCanvas (paint_bg);
      set_background (bg);
      bg.show ();

      notify["state"].connect (() => {
        bg.update ();

        if (get_text () is Ctk.Text)
          {
            if (state == Ctk.ActorState.STATE_NORMAL ||
                state == Ctk.ActorState.STATE_PRELIGHT)
              {
                get_text ().color = { 255, 255, 255, 255 };
              }
            else
                get_text ().color = { 50, 50, 50, 200 };
          }

        if (glow is Ctk.EffectGlow)
          glow.set_invalidate_effect_cache (true);
      });

      outline_func = rounded_rect;
    }

    public void rounded_rect (Cairo.Context cr, int width, int height)
    {
      var padding = 1;
      var x = padding;
      var y = padding;
      var radius = 7;

      width -= padding *2;
      height -= padding * 2;

      cr.move_to  (x, y + radius);
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
      cr.close_path ();    }

    private void paint_bg (Cairo.Context cr, int width, int height)
    {

      cr.set_operator (Cairo.Operator.CLEAR);
      cr.paint ();

      if (state == Ctk.ActorState.STATE_NORMAL)
        return;

      cr.set_operator (Cairo.Operator.OVER);
      cr.set_line_width (1.5);
      cr.translate (0.5, 0.5);

      outline_func (cr, width, height);

      if (state == Ctk.ActorState.STATE_NORMAL)
        {
         cr.set_source_rgba (1.0, 1.0, 1.0, 0.0);
        }
      else if (state == Ctk.ActorState.STATE_PRELIGHT)
        {
          var pattern = new Cairo.Surface.similar (cr.get_target (),
                                               Cairo.Content.COLOR_ALPHA,
                                               4, 4);
          var context = new Cairo.Context (pattern);
          
          context.set_operator (Cairo.Operator.CLEAR);
          context.paint ();

          context.set_line_width (0.2);
          context.set_operator (Cairo.Operator.OVER);
          context.set_source_rgba (1.0, 1.0, 1.0, 0.85);

          context.move_to (0, 0);
          context.line_to (4, 4);

          context.stroke ();

          var pat = new Cairo.Pattern.for_surface (pattern);
          pat.set_extend (Cairo.Extend.REPEAT);
          cr.set_source (pat);
        }
      else
        {
          cr.set_source_rgba (1.0, 1.0, 1.0, 1.0);
        }
      cr.fill_preserve ();

      cr.set_source_rgba (1.0, 1.0, 1.0, 0.5);
      cr.stroke ();
    }
  }
}
