/* -*- Mode: vala; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/*
 * Copyright (C) 2009 Canonical Ltd
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
 * Authored by Gordon Allott <gord.allott@canonical.com>
 *
 */

namespace Unity.Launcher
{
  const string HONEYCOMB_MASK_FILE = Unity.PKGDATADIR
    + "/honeycomb-mask.png";
  const string MENU_BG_FILE = Unity.PKGDATADIR
    + "/tight_check_4px.png";

  class LauncherChild : ScrollerChild
  {

    private Ctk.Actor processed_icon;
    private ThemeImage active_indicator;
    private ThemeImage running_indicator;
    private Gdk.Pixbuf honeycomb_mask;

    // effects
    private Ctk.EffectDropShadow effect_drop_shadow;

    // animations
    private Clutter.Animation active_indicator_anim;
    private Clutter.Animation running_indicator_anim;

    construct
    {
      load_textures ();
    }

		~LauncherChild ()
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

        processed_icon = new Ctk.Image (48);
        processed_icon.set_size (48, 48);
        processed_icon.set_parent (this);

        notify["icon"].connect (on_icon_changed);
        notify["running"].connect (on_running_changed);
        notify["active"].connect (on_active_changed);

        // just trigger some notifications now to set inital state
        on_running_changed ();
        on_active_changed ();
    }

    /* notifications */
    private void on_icon_changed ()
    {
      if (icon is Gdk.Pixbuf)
        {
          Gdk.Pixbuf scaled_buf;
          if (icon.get_width () > 48 || icon.get_height () > 48)
            scaled_buf = icon.scale_simple (48, 48, Gdk.InterpType.HYPER);
          else
            scaled_buf = icon;

          Gdk.Pixbuf color_buf = new Gdk.Pixbuf (Gdk.Colorspace.RGB, true, 8, 1, 1);
          uint red, green, blue;
          Unity.get_average_color (scaled_buf, out red, out green, out blue);
          unowned uchar[] pixels = color_buf.get_pixels ();
          pixels[0] = (uchar)red;
          pixels[1] = (uchar)green;
          pixels[2] = (uchar)blue;
          pixels[3] = 128;

          var tex = GtkClutter.texture_new_from_pixbuf (scaled_buf);
          var color = GtkClutter.texture_new_from_pixbuf (color_buf);

          processed_icon = new UnityIcon (tex as Clutter.Texture, color as Clutter.Texture);
          processed_icon.set_parent (this);

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
      float nat, min;
      processed_icon.get_preferred_height (for_width, out min, out nat);
      natural_height = nat;
      minimum_height = min;
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



