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
 * Authored by
 *               Neil Jagdish Patel <neil.patel@canonical.com>
 *               Mikkel Kamstrup Erlandsen <mikkel.kamstrup@canonical.com>
 *
 */

using Unity;

namespace Unity.Places
{
  public class PlaceSearchExtraAction : Ctk.Bin
  {
    static const float PADDING = 4.0f;

    private CairoCanvas    bg;
    public  Ctk.Image      image;
    private Clutter.Color  color;
    private Ctk.EffectGlow glow;

    public float destroy_factor {
        get { return _destroy_factor; }
        set { _destroy_factor = value; queue_relayout (); }
    }

    public float resize_factor {
        get { return _resize_factor; }
        set { _resize_factor = value; queue_relayout (); }
    }

    public signal void activated ();

    private float _destroy_factor = 1.0f;
    private float _resize_factor = 1.0f;
    private float _last_width = 0.0f;
    private float _resize_width = 0.0f;

    public PlaceSearchExtraAction ()
    {
      Object (reactive:true);
    }

    construct
    {
      color.alpha = 255;

      padding = { PADDING, PADDING, PADDING, PADDING };

      bg = new CairoCanvas (paint_bg);
      set_background (bg);
      bg.show ();

      glow = new Ctk.EffectGlow ();
      glow.set_color ({ 255, 255, 255, 255 });
      glow.set_factor (1.0f);
      glow.set_margin (5);
      //add_effect (glow);

      image = new Ctk.Image (22);
      add_actor (image);
      image.show ();
    }

    public void set_icon_from_gicon_string (string icon_string)
    {
      PixbufCache.get_default ().set_image_from_gicon_string (image,
                                                              icon_string,
                                                              22);
      glow.set_invalidate_effect_cache (true);
    }

    private override void get_preferred_width (float     for_height,
                                               out float min_width,
                                               out float nat_width)
    {
     float mw, nw;

     base.get_preferred_width (for_height, out mw, out nw);

     min_width = (mw + padding.right + padding.left) * _destroy_factor;
     nat_width = (nw + padding.right + padding.left) * _destroy_factor;

     if (_last_width ==0.0f)
       _last_width = nat_width;

     if (_last_width != nat_width  && _resize_factor == 1.0)
       {
         _resize_factor = 0.0f;
         animate (Clutter.AnimationMode.EASE_OUT_QUAD, 100,
                  "resize_factor", 1.0f);

        _resize_width = _last_width;
        _last_width = nat_width;
       }

     if (_resize_factor != 1.0f)
       {
         min_width = _resize_width + ((min_width - _resize_width) * _resize_factor);
         nat_width = _resize_width + ((nat_width - _resize_width) * _resize_factor);
       }
    }

    private override bool enter_event (Clutter.Event e)
    {
      state = Ctk.ActorState.STATE_PRELIGHT;
      bg.update ();
      return true;
    }

    private override bool leave_event (Clutter.Event e)
    {
      state = Ctk.ActorState.STATE_NORMAL;
      bg.update ();
      return true;
    }

    private override bool button_press_event (Clutter.Event e)
    {
      state = Ctk.ActorState.STATE_SELECTED;
      bg.update ();
      return false;
    }

    private override bool button_release_event (Clutter.Event e)
    {
      state = Ctk.ActorState.STATE_PRELIGHT;
      bg.update ();
      activated ();
      return false;
    }

    private void paint_bg (Cairo.Context cr, int width, int height)
    {
      cr.set_operator (Cairo.Operator.CLEAR);
      cr.paint ();

      cr.set_operator (Cairo.Operator.OVER);
      cr.set_line_width (1.5);
      cr.set_source_rgba (1.0, 1.0, 1.0, 0.0);

      cr.translate (0.5, 0.5);

      var x = 0;
      var y = 0;
      width -= 1;
      height -= 1;
      var radius = 10;

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
      cr.close_path ();

      if (state == Ctk.ActorState.STATE_SELECTED)
        {
          cr.set_source_rgba (1.0, 1.0, 1.0, 1.0);
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
          cr.set_source_rgba (1.0, 1.0, 1.0, 0.0);
        }

      cr.fill_preserve ();

      cr.set_source_rgba (1.0, 1.0, 1.0,
                          state == Ctk.ActorState.STATE_NORMAL ? 0.0 : 0.5);
      cr.stroke ();            
    }
  }
}
