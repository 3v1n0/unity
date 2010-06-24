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
 * Authored by Gordon Allott <gord.allott@canonical.com>
 */
using Cairo;

namespace Unity
{
  /* General colour theory utility functions, could maybe be somewhere else
   * but i guess its fine in here for now - as long as its in the Unity namespace
   * it does not really matter
   */
  /* converts from rgb to hsv colour space */
  public static void rgb_to_hsv (float r, float g, float b,
                                 out float hue, out float sat, out float val)
  {
    float min, max = 0.0f;
    if (r > g)
      max = (r > b) ? r : b;
    else
      max = (g > b) ? g : b;
    if (r < g)
      min = (r < b) ? r : b;
    else
      min = (g < b) ? g : b;

    val = max;

    float delta = max - min;
    if (delta > 0.000001)
      {
        sat = delta / max;
        hue = 0.0f;
        if (r == max)
          {
            hue = (g - b) / delta;
            if (hue < 0.0f)
              hue += 6.0f;
          }
        else if (g == max)
          {
            hue = 2.0f + (b - r) / delta;
          }
        else if (b == max)
          {
            hue = 4.0f + (r - g) / delta;
          }
        hue /= 6.0f;
      }
    else
      {
        sat = 0.0f;
        hue = 0.0f;
      }
  }

  public static void hsv_to_rgb (float hue, float sat, float val,
                                 out float r, out float g, out float b)
  {
    int    i;
    float f, w, q, t;

    if (sat == 0.0)
      {
       r = g = b = val;
      }
    else
     {
       if (hue == 1.0)
          hue = 0.0f;

       hue *= 6.0f;

       i = (int) hue;
       f = hue - i;
       w = val * (1.0f - sat);
       q = val * (1.0f - (sat * f));
       t = val * (1.0f - (sat * (1.0f - f)));

       switch (i)
       {
         case 0:
           r = val;
           g = t;
           b = w;
           break;
         case 1:
           r = q;
           g = val;
           b = w;
           break;
         case 2:
           r = w;
           g = val;
           b = t;
           break;
         case 3:
           r = w;
           g = q;
           b = val;
           break;
         case 4:
           r = t;
           g = w;
           b = val;
           break;
         case 5:
           r = val;
           g = w;
           b = q;
           break;
       }
     }
  }

  private static uint pixbuf_check_threshold (Gdk.Pixbuf source,
                                              int x1, int y1, int x2, int y2, float threshold)
  {
    int num_channels = source.get_n_channels ();
    int width = source.get_width ();
    int rowstride = source.get_rowstride ();
    uint total_visible_pixels = 0;
    unowned uchar[] pixels = source.get_pixels ();
    if (source.get_colorspace () != Gdk.Colorspace.RGB ||
        source.get_bits_per_sample () != 8 ||
        !source.get_has_alpha () ||
        num_channels != 4)
      {
        // we can't deal with this pixbuf =\
        warning ("pixbuf is not in a friendly format, can not work with it");
        return 0;
      }

    uint i = 0;
    for (int yi = y1; yi < y2; yi++)
      {
        for (int xi = x1; xi < x2; xi++)
          {
            float pixel = pixels[(i + (xi*4)) + 3] / 255.0f;
            if (pixel > threshold)
              total_visible_pixels += 1;
          }
        i = yi * (width * 4) + rowstride;
      }

    return total_visible_pixels;
  }

  public static bool pixbuf_is_tile (Gdk.Pixbuf source)
  {
    bool is_tile = false;
    int num_channels = source.get_n_channels ();
    int width = source.get_width ();
    int height = source.get_height ();
    uint total_visible_pixels = 0;
    if (source.get_colorspace () != Gdk.Colorspace.RGB ||
        source.get_bits_per_sample () != 8 ||
        !source.get_has_alpha () ||
        num_channels != 4)
      {
        // we can't deal with this pixbuf =\
        warning ("pixbuf is not in a friendly format, can not work with it");
        return false;
      }

    int height_3 = height / 3;
    int width_3 = width / 3;

    total_visible_pixels = pixbuf_check_threshold (source, width_3, 0, width_3*2, 3, 0.1f);
    total_visible_pixels += pixbuf_check_threshold (source, width_3, height-3, width_3*2, 3, 0.1f);
    total_visible_pixels += pixbuf_check_threshold (source, 0, height_3, 3, height_3*2, 0.1f);
    total_visible_pixels += pixbuf_check_threshold (source, width - 3, height_3, 3, height_3*2, 0.1f);

    int max_pixels = (width_3 * 6 + height_3 * 6) / 3;

    if (total_visible_pixels / max_pixels > 0.33333)
      is_tile = true;

    return is_tile;
  }

  public static void get_average_color (Gdk.Pixbuf source, out uint red, out uint green, out uint blue)
  {
    int num_channels = source.get_n_channels ();
    int width = source.get_width ();
    int height = source.get_height ();
    int rowstride = source.get_rowstride ();
    float r, g, b, a, hue, sat, val;
    unowned uchar[] pixels = source.get_pixels ();
    if (source.get_colorspace () != Gdk.Colorspace.RGB ||
        source.get_bits_per_sample () != 8 ||
        !source.get_has_alpha () ||
        num_channels != 4)
      {
        // we can't deal with this pixbuf :-\
        red = 255;
        green = 255;
        blue = 255;
        return;
      }

    double r_total, g_total, b_total, rs_total, gs_total, bs_total;
    r_total = g_total = b_total = 0.0;
    rs_total = gs_total = bs_total = 0.0;

    int i = 0;
    uint total_caught_pixels = 1;
    for (int y = 0; y < height; y++)
    {
      for (int x = 0; x < width; x++)
      {
        int pix_index = i + (x*4);
        r = pixels[pix_index + 0] / 255.0f;
        g = pixels[pix_index + 1] / 255.0f;
        b = pixels[pix_index + 2] / 255.0f;
        a = pixels[pix_index + 3] / 255.0f;

        if (a < 0.5)
          continue;

        rgb_to_hsv (r, g, b, out hue, out sat, out val);
        rs_total += r;
        gs_total += g;
        bs_total += b;

        if (sat <= 0.33)
          continue;

        // we now have the saturation and value! wewt.
        r_total += r;
        g_total += g;
        b_total += b;

        total_caught_pixels += 1;
      }
      i = y * (width * 4) + rowstride;
    }
    // okay we should now have a large value in our totals
    r_total = r_total / uint.max (total_caught_pixels, 1);
    g_total = g_total / uint.max (total_caught_pixels, 1);
    b_total = b_total / uint.max (total_caught_pixels, 1);

    // get a new super saturated value based on our totals
    if (total_caught_pixels <= 20)
      {
        rgb_to_hsv ((float)rs_total, (float)gs_total, (float)bs_total, out hue, out sat, out val);
        sat = 0.0f;
      }
    else
      {
        rgb_to_hsv ((float)r_total, (float)g_total, (float)b_total, out hue, out sat, out val);
        sat = Math.fminf (sat * 0.7f, 1.0f);
        val = Math.fminf (val * 1.4f, 1.0f);
      }

    hsv_to_rgb (hue, sat, val, out r, out g, out b);

    red = (uint)(r * 255);
    green = (uint)(g * 255);
    blue = (uint)(b * 255);
  }

  /* just a layered actor that layers the different components of our
   * icon on top of each other
   */
  private static Clutter.Texture? unity_icon_bg_layer; //background
  private static Clutter.Texture? unity_icon_fg_layer; //foreground
  private static Clutter.Texture? unity_icon_mk_layer; //mask

  public class UnityIcon : Ctk.Actor
  {
    public Clutter.Texture? icon {get; construct;}
    public Clutter.Texture? bg_color {get; construct;}

    private Cogl.Material bg_mat;
    private Cogl.Material fg_mat;
    private Cogl.Material icon_material;
    private Cogl.Material bgcol_material;

    public float rotation = 0.0f;


    public UnityIcon (Clutter.Texture? icon, Clutter.Texture? bg_tex)
    {
      Object (icon: icon, bg_color: bg_tex);
    }

    construct
    {
      icon_material = new Cogl.Material ();
      bgcol_material = new Cogl.Material ();

      if (!(unity_icon_bg_layer is Clutter.Texture))
        {
          unity_icon_bg_layer = new ThemeImage ("prism_icon_background");
          unity_icon_fg_layer = new ThemeImage ("prism_icon_foreground");
          unity_icon_mk_layer = new ThemeImage ("prism_icon_mask");
        }

      if (icon is Clutter.Texture)
        {
          icon.set_parent (this);
          var icon_mat = new Cogl.Material ();
          Cogl.Texture icon_tex = (Cogl.Texture)(icon.get_cogl_texture ());
          Cogl.Texture mask_tex = (Cogl.Texture)(unity_icon_mk_layer.get_cogl_texture ());
          icon_mat.set_layer (0, icon_tex);
          icon_mat.set_layer_filters (1, Cogl.MaterialFilter.LINEAR, Cogl.MaterialFilter.LINEAR);
          icon_mat.set_layer (1, mask_tex);
          icon_material = icon_mat;
        }
      if (bg_color is Clutter.Texture)
        {
          bg_color.set_parent (this);
          bgcol_material = new Cogl.Material ();
          Cogl.Texture color = (Cogl.Texture)(bg_color.get_cogl_texture ());
          Cogl.Texture mask_tex = (Cogl.Texture)(unity_icon_mk_layer.get_cogl_texture ());
          bgcol_material.set_layer (0, color);
          bgcol_material.set_layer_filters (1, Cogl.MaterialFilter.LINEAR, Cogl.MaterialFilter.LINEAR);
          bgcol_material.set_layer (1, mask_tex);
        }

        var mat = new Cogl.Material ();
        Cogl.Texture tex = (Cogl.Texture)(unity_icon_bg_layer.get_cogl_texture ());
        mat.set_layer (0, tex);
        bg_mat = mat;

        mat = new Cogl.Material ();
        tex = (Cogl.Texture)(unity_icon_fg_layer.get_cogl_texture ());
        mat.set_layer (0, tex);
        fg_mat = mat;
    }

    public override void get_preferred_width (float for_height,
                                              out float minimum_width,
                                              out float natural_width)
    {
      natural_width = minimum_width = 50;
    }

    public override void get_preferred_height (float for_width,
                                               out float minimum_height,
                                               out float natural_height)
    {
      natural_height = minimum_height = 50;
    }

    public override void allocate (Clutter.ActorBox box, Clutter.AllocationFlags flags)
    {
      base.allocate (box, flags);
      if (icon is Clutter.Texture)
        icon.allocate (box, flags);
      if (bg_color is Clutter.Texture)
        bg_color.allocate (box, flags);
    }

    public override void pick (Clutter.Color color)
    {
      set_effects_painting (true);
      var mat = new Cogl.Material ();
      mat.set_color4ub (color.red, color.green, color.blue, color.alpha);
      Cogl.rectangle (0, 0, 48, 48);
      base.pick (color);
      set_effects_painting (false);
    }

    /* The closest most horrible thing you will ever see in vala. its basically
     * C code... - oh well it works
     */
    public static void paint_real (Clutter.Actor actor)
    {
      UnityIcon self = actor as UnityIcon;
      float p1_x, p1_y;
      float p2_x, p2_y;
      float p3_x, p3_y;
      float p4_x, p4_y;
      float z, w;

      Clutter.ActorBox box = Clutter.ActorBox ();
      self.get_stored_allocation (out box);

      Cogl.Matrix modelview = Cogl.Matrix.identity (); //model view matrix
      Cogl.Matrix projection = Cogl.Matrix.identity (); // projection matrix
      projection.perspective (60.0f, 1.0f, 0.1f, 100.0f);
      modelview.translate (0.0f, 0.0f, -44.0f - Math.fabsf (self.rotation / 360.0f) * 100);
      modelview.rotate (self.rotation, 1.0f, 0.0f, 0.0f);

      Cogl.Matrix viewmatrix = Cogl.Matrix.multiply (projection, modelview);

      p1_x = -25.0f; p1_y = -25.0f;
      p2_x =  25.0f; p2_y = -25.0f;
      p3_x =  25.0f; p3_y =  25.0f;
      p4_x = -25.0f; p4_y =  25.0f;
      z = 0.0f;
      w = 1.0f;

      viewmatrix.transform_point (out p1_x, out p1_y, out z, out w); p1_x /= w; p1_y /= w; z = 0.0f; w = 1.0f;
      viewmatrix.transform_point (out p2_x, out p2_y, out z, out w); p2_x /= w; p2_y /= w; z = 0.0f; w = 1.0f;
      viewmatrix.transform_point (out p3_x, out p3_y, out z, out w); p3_x /= w; p3_y /= w; z = 0.0f; w = 1.0f;
      viewmatrix.transform_point (out p4_x, out p4_y, out z, out w); p4_x /= w; p4_y /= w; z = 0.0f; w = 1.0f;


      //transform into screen co-ordinates
      p1_x = (50 * (p1_x + 1) / 2);
      p1_y =  48 + (50 * (p1_y - 1) / 2);

      p2_x = (50 * (p2_x + 1) / 2);
      p2_y =  48 + (50 * (p2_y - 1) / 2);

      p3_x = (50 * (p3_x + 1) / 2);
      p3_y =  48 + (50 * (p3_y - 1) / 2);

      p4_x = (50 * (p4_x + 1) / 2);
      p4_y =  48 + (50 * (p4_y - 1) / 2);

      if (Math.fabsf (self.rotation) <= 3.0)
        {
          // floor all the values. we don't floor when its rotated because then
          // we lose subpixel accuracy and things look like a 1991 video game
          p1_x = Math.floorf (p1_x); p1_y = Math.floorf (p1_y);
          p2_x = Math.floorf (p2_x); p2_y = Math.floorf (p2_y);
          p3_x = Math.floorf (p3_x); p3_y = Math.floorf (p3_y);
          p4_x = Math.floorf (p4_x); p4_y = Math.floorf (p4_y);
        }

      Cogl.TextureVertex[4] points = {
        Cogl.TextureVertex () {
          x = p1_x,
          y = p1_y,
          z = 0.0f,
          tx = 0.0f,
          ty = 0.0f,
          color = Cogl.Color () {
            red = 0xff,
            green = 0xff,
            blue = 0xff,
            alpha = 0xff
          }
        },
        Cogl.TextureVertex () {
          x = p2_x,
          y = p2_y,
          z = 0.0f,
          tx = 1.0f,
          ty = 0.0f,
          color = Cogl.Color () {
            red = 0xff,
            green = 0xff,
            blue = 0xff,
            alpha = 0xff
          }
        },
        Cogl.TextureVertex () {
          x = p3_x,
          y = p3_y,
          z = 0.0f,
          tx = 1.0f,
          ty = 1.0f,
          color = Cogl.Color () {
            red = 0xff,
            green = 0xff,
            blue = 0xff,
            alpha = 0xff
          }
        },
        Cogl.TextureVertex () {
          x = p4_x,
          y = p4_y,
          z = 0.0f,
          tx = 0.0f,
          ty = 1.0f,
          color = Cogl.Color () {
            red = 0xff,
            green = 0xff,
            blue = 0xff,
            alpha = 0xff
          }
        }
      };

      uchar opacity = self.get_paint_opacity ();

      self.bg_mat.set_color4ub (opacity, opacity, opacity, opacity);
      self.bgcol_material.set_color4ub (opacity, opacity, opacity, opacity);
      self.icon_material.set_color4ub (opacity, opacity, opacity, opacity);
      self.fg_mat.set_color4ub (opacity, opacity, opacity, opacity);

      if (self.bg_color is Clutter.Texture)
        {
          Cogl.set_source (self.bgcol_material);
          Cogl.polygon (points, true);
        }
      else
        {
          Cogl.set_source (self.bg_mat);
          Cogl.polygon (points, true);
        }
      if (self.icon is Clutter.Texture)
        {
          // we also need to transform the smaller (potentially) icon
          int base_width, base_height;
          float xpad, ypad;
          self.icon.get_base_size (out base_width, out base_height);
          xpad = 1 + (box.get_width () - base_width) / 2.0f;
          ypad = ((box.get_height () - base_height) / 2.0f) - 1;

          p1_x = -25.0f; p1_y = -25.0f;
          p2_x =  25.0f; p2_y = -25.0f;
          p3_x =  25.0f; p3_y =  25.0f;
          p4_x = -25.0f; p4_y =  25.0f;
          z = 0.0f;
          w = 1.0f;

          viewmatrix.transform_point (out p1_x, out p1_y, out z, out w); p1_x /= w; p1_y /= w; z = 0.0f; w = 1.0f;
          viewmatrix.transform_point (out p2_x, out p2_y, out z, out w); p2_x /= w; p2_y /= w; z = 0.0f; w = 1.0f;
          viewmatrix.transform_point (out p3_x, out p3_y, out z, out w); p3_x /= w; p3_y /= w; z = 0.0f; w = 1.0f;
          viewmatrix.transform_point (out p4_x, out p4_y, out z, out w); p4_x /= w; p4_y /= w; z = 0.0f; w = 1.0f;

          //transform into screen co-ordinates
          p1_x = xpad + (base_width * (p1_x + 1) / 2);
          p1_y = (48 - ypad) + (base_height * (p1_y - 1) / 2);

          p2_x = xpad + (base_width * (p2_x + 1) / 2);
          p2_y = (48 - ypad) + (base_height * (p2_y - 1) / 2);

          p3_x = xpad + (base_width * (p3_x + 1) / 2);
          p3_y = (48 - ypad) + (base_height * (p3_y - 1) / 2);

          p4_x = xpad + (base_width * (p4_x + 1) / 2);
          p4_y = (48 - ypad) + (base_height * (p4_y - 1) / 2);

          if (Math.fabsf (self.rotation) <= 3.0)
            {
              // floor all the values. we don't floor when its rotated because then
              // we lose subpixel accuracy and things look like a 1991 video game
              p1_x = Math.floorf (p1_x); p1_y = Math.floorf (p1_y);
              p2_x = Math.floorf (p2_x); p2_y = Math.floorf (p2_y);
              p3_x = Math.floorf (p3_x); p3_y = Math.floorf (p3_y);
              p4_x = Math.floorf (p4_x); p4_y = Math.floorf (p4_y);
            }

          Cogl.TextureVertex[4] icon_points = {
            Cogl.TextureVertex () {
              x = p1_x,
              y = p1_y,
              z = 0.0f,
              tx = 0.0f,
              ty = 0.0f,
              color = Cogl.Color () {
                red = 0xff,
                green = 0xff,
                blue = 0xff,
                alpha = 0xff
              }
            },
            Cogl.TextureVertex () {
              x = p2_x,
              y = p2_y,
              z = 0.0f,
              tx = 1.0f,
              ty = 0.0f,
              color = Cogl.Color () {
                red = 0xff,
                green = 0xff,
                blue = 0xff,
                alpha = 0xff
              }
            },
            Cogl.TextureVertex () {
              x = p3_x,
              y = p3_y,
              z = 0.0f,
              tx = 1.0f,
              ty = 1.0f,
              color = Cogl.Color () {
                red = 0xff,
                green = 0xff,
                blue = 0xff,
                alpha = 0xff
              }
            },
            Cogl.TextureVertex () {
              x = p4_x,
              y = p4_y,
              z = 0.0f,
              tx = 0.0f,
              ty = 1.0f,
              color = Cogl.Color () {
                red = 0xff,
                green = 0xff,
                blue = 0xff,
                alpha = 0xff
              }
            }
          };

          int width, height;
          self.icon.get_base_size (out width, out height);

          xpad = (box.get_width () - width) / 2.0f;
          ypad = (box.get_height () - height) / 2.0f;

          Cogl.set_source (self.icon_material);
          Cogl.polygon (icon_points, true);
        }

      Cogl.set_source (self.fg_mat);
      Cogl.polygon (points, true);
    }

    public override void paint ()
    {
      /* we need a beter way of doing this effects stuff in vala, its horrible
       * to do, must have a think...
       */
      unowned SList<Ctk.Effect> effects = get_effects ();
      if (!get_effects_painting () && effects != null)
        {
          unowned SList<Ctk.Effect> e;
          set_effects_painting (true);
          for (e = effects; e != null; e = e.next)
            {
              Ctk.Effect effect = e.data;
              bool last_effect = (e.next != null) ? false : true;
              effect.paint (paint_real, last_effect);
            }

          set_effects_painting (false);
        }
      else
        {
          paint_real (this);
        }
    }

    public override void map ()
    {
      base.map ();
      if (icon is Clutter.Actor)
        icon.map ();
    }

    public override void unmap ()
    {
      base.unmap ();
      icon.unmap ();
    }
  }
}
