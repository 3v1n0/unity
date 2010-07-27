/*
 *      scrollerchild.vala.vala
 *      Copyright (C) 2010 Canonical Ltd
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 *
 *
 *      Authored by Gordon Allott <gord.allott@canonical.com>
 */


namespace Unity.Launcher
{
  const string HONEYCOMB_MASK_FILE = Unity.PKGDATADIR
    + "/honeycomb-mask.png";
  const string MENU_BG_FILE = Unity.PKGDATADIR
    + "/tight_check_4px.png";

  const float WIGGLE_SIZE = 5; // how many degree's to wiggle on either side.
  const int WIGGLE_FREQUENCY = 5; // x times a second
  const int WIGGLE_RUN_LENGTH = 5000; // 5 seconds of wiggle
  const int WIGGLE_PAUSE_LENGTH = 20; // followed by 20 seconds of no wiggle

  private enum AnimState {
    RISING,
    LOOPING,
    FALLING,
    STOPPED
  }

  public enum PinType {
    UNPINNED,
    PINNED,
    ALWAYS,
    NEVER
  }

  public class ScrollerChild : Ctk.Actor
  {
    public Gdk.Pixbuf icon {get; set;}
    public PinType pin_type;
    public float position {get; set;}
    public bool running {get; set;}
    public bool active {get; set;}
    public bool needs_attention {get; set;}
    public bool activating {get; set;}
    public float rotation {get; set;}
    public ScrollerChildController controller; // this sucks. shouldn't be here, can't help it.

    public string to_string ()
    {
      return "A scroller child; running: %s, active: %s, position: %f, opacity %f".printf (
                (running) ? "yes" : "no",
                (active) ? "yes" : "no",
                position,
                opacity);
    }

    private UnityIcon processed_icon;
    private ThemeImage active_indicator;
    private ThemeImage running_indicator;
    private Gdk.Pixbuf honeycomb_mask;

    // effects
    private Ctk.EffectDropShadow effect_drop_shadow;
    private Ctk.EffectGlow effect_icon_glow;

    // animations
    private Clutter.Animation active_indicator_anim;
    private Clutter.Animation running_indicator_anim;
    private Clutter.Timeline  wiggle_timeline;
    private Clutter.Timeline  glow_timeline;
    private Clutter.Timeline  rotate_timeline;
    private AnimState glow_state;
    private AnimState wiggle_state;
    private AnimState rotate_state;

    private float old_rotate_value = 0.0f;

    construct
    {
      load_textures ();
      position = 0.0f;

      //icon glow
      glow_timeline = new Clutter.Timeline (1);
      wiggle_timeline = new Clutter.Timeline (1);
      rotate_timeline = new Clutter.Timeline (1);

      glow_timeline.new_frame.connect (on_glow_timeline_new_frame);
      wiggle_timeline.new_frame.connect (on_wiggle_timeline_new_frame);
      rotate_timeline.new_frame.connect (on_rotate_timeline_new_frame);

      notify["rotation"].connect (on_rotation_changed);
    }

    ~ScrollerChild ()
    {
      running_indicator.unparent ();
      active_indicator.unparent ();
    }

    /* private methods */
    private void load_textures ()
    {
      active_indicator = new ThemeImage ("application-selected");
      running_indicator = new ThemeImage ("application-running");

      active_indicator.set_parent (this);
      running_indicator.set_parent (this);
      active_indicator.set_opacity (0);
      running_indicator.set_opacity (0);

      try
        {
          honeycomb_mask = new Gdk.Pixbuf.from_file(HONEYCOMB_MASK_FILE);
        }
      catch (Error e)
        {
          warning ("Unable to load asset %s: %s",
                   HONEYCOMB_MASK_FILE,
                   e.message);
        }

        processed_icon = new UnityIcon (null, null);
        processed_icon.set_size (48, 48);
        processed_icon.set_parent (this);

        notify["icon"].connect (on_icon_changed);
        notify["running"].connect (on_running_changed);
        notify["active"].connect (on_active_changed);
        notify["activating"].connect (on_activating_changed);
        notify["needs-attention"].connect (on_needs_attention_changed);

        // just trigger some notifications now to set inital state
        on_running_changed ();
        on_active_changed ();
        on_rotation_changed ();
    }

    public Clutter.Actor get_content ()
    {
      return processed_icon;
    }

    /* alpha helpers */
    private static float get_ease_out_sine (float alpha)
    {
      return (float)(Math.sin ((Math.PI_2 * alpha)));
    }

    private static float get_circular_alpha (float alpha)
    {
      //float sine = (float)(Math.sin (-Math.PI + (alpha * (Math.PI * 2))));
      var sine = Math.sin ((alpha * (Math.PI * 2)) - Math.PI);
      return Math.fmaxf(((float)sine / 2.0f) + 0.5f, 0.0f);;
    }
    /* animation callbacks */

    private void on_rotate_timeline_new_frame ()
    {
      float progress = (float)rotate_timeline.get_progress ();
      switch (rotate_state)
        {
          case AnimState.RISING:
            rotate_anim_rising (progress);
            break;

          case AnimState.STOPPED:
            rotate_timeline.stop ();
            break;
        }
      processed_icon.do_queue_redraw ();
    }

    private void rotate_anim_rising (float progress)
    {
      progress = get_ease_out_sine (progress);
      var diff = rotation - old_rotate_value;
      float rotate_val = old_rotate_value + (progress * diff);

      processed_icon.rotation = rotate_val;
      if (progress >= 1.0)
        {
          rotate_state = AnimState.STOPPED;
          rotate_timeline.stop ();
        }
    }

    public void force_rotation_jump (float degrees)
    {
      processed_icon.rotation = degrees;
      rotation = degrees;
      rotate_state = AnimState.STOPPED;
      rotate_timeline.stop ();
      do_queue_redraw ();
    }

    private void on_glow_timeline_new_frame ()
    {
      float progress = (float)glow_timeline.get_progress ();
      switch (glow_state)
        {
          case AnimState.RISING:
            glow_anim_rising (progress);
            break;

          case AnimState.LOOPING:
            glow_anim_looping (progress);
            break;

          case AnimState.FALLING:
            glow_anim_falling (progress);
            break;

          default:
            glow_state = AnimState.STOPPED;
            glow_timeline.stop ();
            break;
        }

      processed_icon.do_queue_redraw ();
    }

    private float previous_glow_alpha = 0.0f;
    private void glow_anim_rising (float progress)
    {
      progress = get_ease_out_sine (progress);
      effect_icon_glow.set_opacity (progress);
      previous_glow_alpha = progress;
      if (progress >= 1.0)
        {
          glow_state = AnimState.LOOPING;
          glow_timeline.stop ();
          glow_timeline.set_duration (LONG_DELAY);
          glow_timeline.set_loop (true);
          glow_timeline.start ();
          return;
        }
    }

    private void glow_anim_looping (float progress)
    {
      progress = 1.0f - get_circular_alpha (progress);
      effect_icon_glow.set_opacity (progress);
      previous_glow_alpha = progress;
      processed_icon.do_queue_redraw ();
    }

    private void glow_anim_falling (float progress)
    {
      float alpha_length = previous_glow_alpha;
      effect_icon_glow.set_opacity (alpha_length - (progress * alpha_length));

      if (progress >= 1.0)
        {
          glow_state = AnimState.STOPPED;
          glow_timeline.stop ();
          glow_timeline.set_loop (false);
        }
    }

    private void on_wiggle_timeline_new_frame ()
    {
      float progress = (float)wiggle_timeline.get_progress ();

      switch (wiggle_state)
        {
          case AnimState.RISING:
            wiggle_anim_rising (progress);
            break;

          case AnimState.LOOPING:
            wiggle_anim_looping (progress);
            break;

          case AnimState.FALLING:
            wiggle_anim_falling (progress);
            break;

          default:
            wiggle_state = AnimState.STOPPED;
            wiggle_timeline.stop ();
            break;
        }

      processed_icon.do_queue_redraw ();
    }

    private float previous_wiggle_alpha = 0.0f;
    private void wiggle_anim_rising (float progress)
    {
      progress = get_ease_out_sine (progress);
      processed_icon.set_rotation (Clutter.RotateAxis.Z_AXIS, progress * WIGGLE_SIZE,
                                   25.0f, 25.0f, 0.0f);
      previous_wiggle_alpha = progress;
      if (progress >= 1.0)
        {
          wiggle_state = AnimState.LOOPING;
          wiggle_timeline.stop ();
          wiggle_timeline.set_duration (WIGGLE_RUN_LENGTH);
          wiggle_timeline.set_loop (true);
          wiggle_timeline.start ();
          return;
        }
    }

    private void wiggle_anim_looping (float progress)
    {
      if (progress >= 1.0)
        {
          wiggle_state = AnimState.FALLING;
          wiggle_timeline.stop ();
          wiggle_timeline.set_loop (false);
          wiggle_timeline.start ();
        }

      int frequency = WIGGLE_FREQUENCY * (WIGGLE_RUN_LENGTH / 1000);
      progress = get_circular_alpha (Math.fmodf (progress * frequency, 1.0f));
      progress = (1.0f - progress) * 2.0f - 1.0f;
      processed_icon.set_rotation (Clutter.RotateAxis.Z_AXIS, progress * WIGGLE_SIZE,
                                   25.0f, 25.0f, 0.0f);
      processed_icon.do_queue_redraw ();
      previous_wiggle_alpha = progress;


    }

    private bool check_continue_wiggle ()
    {
      if (needs_attention)
        {
          wiggle_timeline.set_duration (500 / WIGGLE_FREQUENCY);
          wiggle_state = AnimState.RISING;
          wiggle_timeline.start ();
        }
      return false;
    }

    private void wiggle_anim_falling (float progress)
    {
      float alpha_length = previous_wiggle_alpha;
      float angle = alpha_length - (progress * alpha_length);
      processed_icon.set_rotation (Clutter.RotateAxis.Z_AXIS, angle,
                                   25.0f, 25.0f, 0.0f);

      if (progress >= 1.0)
        {
          wiggle_state = AnimState.STOPPED;
          wiggle_timeline.stop ();
          wiggle_timeline.set_loop (false);
          GLib.Timeout.add_seconds (WIGGLE_PAUSE_LENGTH, check_continue_wiggle);
        }
    }

    /* notifications */
    private void on_icon_changed ()
    {
      if (icon is Gdk.Pixbuf)
        {
          Gdk.Pixbuf scaled_buf;
          int max_size = 48;
          if (!Unity.pixbuf_is_tile (icon))
            max_size = 32;

          if (icon.get_width () > max_size || icon.get_height () > max_size)
            {
              scaled_buf = icon.scale_simple (max_size, max_size, Gdk.InterpType.HYPER);
            }
          else
            {
              scaled_buf = icon;
            }

          Gdk.Pixbuf color_buf = new Gdk.Pixbuf (Gdk.Colorspace.RGB, true, 8, 1, 1);
          uint red, green, blue;
          Unity.get_average_color (scaled_buf, out red, out green, out blue);
          unowned uchar[] pixels = color_buf.get_pixels ();
          pixels[0] = (uchar)red;
          pixels[1] = (uchar)green;
          pixels[2] = (uchar)blue;
          pixels[3] = 255;

          var tex = GtkClutter.texture_new_from_pixbuf (scaled_buf);
          var color = GtkClutter.texture_new_from_pixbuf (color_buf);

          processed_icon = new UnityIcon (tex as Clutter.Texture, color as Clutter.Texture);
          processed_icon.set_parent (this);
          processed_icon.rotation = rotation;

          this.effect_drop_shadow = new Ctk.EffectDropShadow (5.0f, 0, 2);
          effect_drop_shadow.set_opacity (0.4f);
          this.effect_drop_shadow.set_margin (5);
          this.processed_icon.add_effect (effect_drop_shadow);

          do_queue_redraw ();
        }
    }

    private void on_running_changed ()
    {
      uint target_opacity = 0;
      if (running)
        target_opacity = 255;

      if (running_indicator_anim is Clutter.Animation)
        running_indicator_anim.completed ();

      running_indicator_anim = running_indicator.animate (Clutter.AnimationMode.EASE_IN_OUT_SINE,
                                                          SHORT_DELAY,
                                                          "opacity", target_opacity);
    }

    private void on_active_changed ()
    {
      uint target_opacity = 0;
      if (active)
        target_opacity = 255;

      if (active_indicator_anim is Clutter.Animation)
        active_indicator_anim.completed ();
      active_indicator_anim = active_indicator.animate (Clutter.AnimationMode.EASE_IN_OUT_SINE,
                                                        SHORT_DELAY,
                                                        "opacity", target_opacity);
    }

    private void on_rotation_changed ()
    {
      old_rotate_value = processed_icon.rotation;

      if (rotate_timeline is Clutter.Timeline == false)
        return;

      if (rotate_timeline.is_playing ())
        {
          rotate_timeline.stop ();
          processed_icon.rotation = old_rotate_value;
        }

      rotate_timeline.set_duration (300);
      rotate_state = AnimState.RISING;
      rotate_timeline.start ();
    }

    private void on_activating_changed ()
    {
      if (glow_timeline.is_playing () && activating == false)
        {
          glow_timeline.stop ();
          glow_timeline.set_duration (SHORT_DELAY);
          glow_state = AnimState.FALLING;
          glow_timeline.start ();
        }
      else if (glow_timeline.is_playing () == false && activating)
        {
          effect_icon_glow = new Ctk.EffectGlow ();
          Clutter.Color c = Clutter.Color () {
            red = 255,
            green = 255,
            blue = 255,
            alpha = 255
          };
          effect_icon_glow.set_background_texture (honeycomb_mask);
          effect_icon_glow.set_color (c);
          effect_icon_glow.set_opacity (1.0f);
          processed_icon.add_effect (effect_icon_glow);
          effect_icon_glow.set_margin (6);

          glow_timeline.set_duration (SHORT_DELAY);
          glow_state = AnimState.RISING;
          glow_timeline.start ();
        }
    }

    private void on_needs_attention_changed ()
    {
      if (needs_attention && wiggle_timeline.is_playing () == false)
        {
          //start wiggling
          wiggle_timeline.set_duration (500 / WIGGLE_FREQUENCY);
          wiggle_state = AnimState.RISING;
          wiggle_timeline.start ();
        }
      else if (needs_attention == false && wiggle_timeline.is_playing ())
        {
          //stop wiggling
          wiggle_timeline.stop ();
          wiggle_timeline.set_duration (500 / WIGGLE_FREQUENCY);
          wiggle_state = AnimState.FALLING;
          wiggle_timeline.start ();
        }
    }

    /* clutter overrides */
    public override void get_preferred_width (float for_height,
                                                out float minimum_width,
                                                out float natural_width)
    {
      float nat, min;
      processed_icon.get_preferred_width (for_height, out min, out nat);
      natural_width = nat;
      minimum_width = min;

      running_indicator.get_preferred_width (for_height, out min, out nat);
      natural_width += nat;

      active_indicator.get_preferred_width (for_height, out min, out nat);
      natural_width += nat;
    }

    public override void get_preferred_height (float for_width,
                                               out float minimum_height,
                                               out float natural_height)
    {
      natural_height = 48;
      minimum_height = 48;
    }

    public override void allocate (Clutter.ActorBox box, Clutter.AllocationFlags flags)
    {
      float x, y;
      x = 0;
      y = 0;
      base.allocate (box, flags);

      Clutter.ActorBox child_box = Clutter.ActorBox ();

      //allocate the running indicator first
      float width, height, n_width, n_height;
      running_indicator.get_preferred_width (58, out n_width, out width);
      running_indicator.get_preferred_height (58, out n_height, out height);
      child_box.x1 = 0;
      child_box.y1 = (box.get_height () - height) / 2.0f;
      child_box.x2 = child_box.x1 + width;
      child_box.y2 = child_box.y1 + height;
      running_indicator.allocate (child_box, flags);
      x += child_box.get_width ();

      //allocate the icon
      processed_icon.get_preferred_width (48, out width, out n_width);
      processed_icon.get_preferred_height (48, out height, out n_height);
      child_box.x1 = (box.get_width () - width) / 2.0f;
      child_box.y1 = y;
      child_box.x2 = child_box.x1 + 48;
      child_box.y2 = child_box.y1 + height;
      processed_icon.allocate (child_box, flags);

      //allocate the active indicator
      active_indicator.get_preferred_width (48, out n_width, out width);
      active_indicator.get_preferred_height (48, out n_height, out height);
      child_box.x1 = box.get_width () - width;
      child_box.y1 = (box.get_height () - height) / 2.0f;
      child_box.x2 = child_box.x1 + width;
      child_box.y2 = child_box.y1 + height;
      active_indicator.allocate (child_box, flags);

    }

    public override void pick (Clutter.Color color)
    {
      base.pick (color);
    }

    public override void paint ()
    {
      active_indicator.paint ();
      running_indicator.paint ();

      processed_icon.paint ();
    }

    public override void map ()
    {
      base.map ();
      running_indicator.map ();
      active_indicator.map ();
      processed_icon.map ();
    }

    public override void unmap ()
    {
      base.unmap ();
      running_indicator.unmap ();
      active_indicator.unmap ();
      processed_icon.unmap ();
    }
  }
}
