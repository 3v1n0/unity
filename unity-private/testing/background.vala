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
 * Authored by Neil Jagdish Patel <neil.patel@canonical.com>
 *
 * FIXME: wallpaper-slideshows are not yet supported by this class
 */

namespace Unity.Testing
{
  public class Background : Ctk.Bin
  {
    private string BG_DIR        = "/desktop/gnome/background";
    private string BG_PIC_OPTS   = "/desktop/gnome/background/picture_options";
    private string BG_SHADE_TYPE = "/desktop/gnome/background/color_shading_type";
    private string BG_PRIM_COL   = "/desktop/gnome/background/primary_color";
    private string BG_SEC_COL    = "/desktop/gnome/background/secondary_color";
    private string BG_FILENAME   = "/desktop/gnome/background/picture_filename";

    private string DEFAULT_PIC_OPTS   = "none";
    private string DEFAULT_SHADE_TYPE = "solid";
    private string DEFAULT_AMBIANCE_COL = "#2C2C00001E1E"; // Ubuntu Dark Purple
    private string DEFAULT_FILENAME   = "/usr/share/backgrounds/warty-final.png";

    private Clutter.CairoTexture bg_gradient;
    private Clutter.Texture      bg_image;
    private Gnome.BG             gbg;
    private GConf.Client         client;

    private string pic_opts;
    private string shade_type;
    private string prim_col;
    private string sec_col;
    private string filename;

    public Background ()
    {
      Object ();
    }

    construct
    {
      START_FUNCTION ();

      this.gbg = new Gnome.BG ();
      this.client = GConf.Client.get_default ();

      /* register monitoring background-directory with gconf */
      try
        {
          this.client.add_dir (this.BG_DIR, GConf.ClientPreloadType.NONE);
        }
      catch (GLib.Error e)
        {
          warning ("Background: Unable to monitor background-settings: %s",
                   e.message);
        }

      /* load values from gconf */
      try
        {
          this.pic_opts = this.client.get_string (this.BG_PIC_OPTS);
        }
      catch (Error e)
        {
          this.pic_opts = DEFAULT_PIC_OPTS;
        }

      try
        {
          this.shade_type = this.client.get_string (this.BG_SHADE_TYPE);
        }
      catch (Error e)
        {
          this.shade_type = DEFAULT_SHADE_TYPE;
        }

      try
        {
          this.prim_col = this.client.get_string (this.BG_PRIM_COL);
        }
      catch (Error e)
        {
          this.prim_col = DEFAULT_AMBIANCE_COL;
        }

      try
        {
          this.sec_col = this.client.get_string (this.BG_SEC_COL);
        }
      catch (Error e)
        {
          this.sec_col = DEFAULT_AMBIANCE_COL;
        }

      try
        {
          this.filename = this.client.get_string (this.BG_FILENAME);
        }
      catch (Error e)
        {
          this.filename = DEFAULT_FILENAME;
        }

      /* register monitor */
      try
       {
          this.client.notify_add (this.BG_DIR, this._on_gconf_changed);
        }
      catch (Error e)
        {
          warning ("Background: Unable to monitor background: %s", e.message);
        }

      this.bg_gradient = new Clutter.CairoTexture (1, 1);
      this.bg_gradient.name = "bg_gradient";
      this.bg_image = new Clutter.Texture ();
      this.bg_image.name = "bg_image";

      // setting async. loading to TRUE here, requires hooking up to
      // "allocation-changed" for the texture-image, otherwise we cannot get
      // valid values for a texture-images base-size, which is needed to
      // correctly handle image-placement... this sucks balls!!!
      this.bg_image.set_load_async (true);
      this.bg_image.load_finished.connect (_handle_image_placement);

      this.gbg.load_from_preferences (client);

      // this is needed for the bg_gradient to have the correct size
      this.allocation_changed.connect (_on_allocation_changed);

      END_FUNCTION ();
    }

    private void
    _handle_image_placement ()
    {
      Gnome.BGPlacement placement = this.gbg.get_placement ();

      switch (placement)
        {
          case Gnome.BGPlacement.TILED:
            {
              this.bg_image.set_repeat (true, true);
              this.bg_image.set_sync_size (true);
              this.x = 0.0f;
              this.y = 0.0f;
            }
          break;

          case Gnome.BGPlacement.ZOOMED:
            {
              this.bg_image.set_repeat (false, false);
              this.bg_image.set_sync_size (false);
              this.bg_image.set_keep_aspect_ratio (true);
              this.x = 0.0f;
              this.y = 0.0f;
            }
          break;

          case Gnome.BGPlacement.CENTERED:
            {
              this.bg_image.set_repeat (false, false);
              this.bg_image.set_sync_size (false);
              this.bg_image.set_keep_aspect_ratio (false);
              this.x = 0.0f;
              this.y = 0.0f;
            }
          break;

          case Gnome.BGPlacement.SCALED:
            {
              this.bg_image.set_keep_aspect_ratio (true);
              this.bg_image.set_sync_size (false);
              this.x = 0.0f;
              this.y = 0.0f;
            }
          break;

          case Gnome.BGPlacement.FILL_SCREEN:
            {
              this.bg_image.set_keep_aspect_ratio (false);
              this.x = 0.0f;
              this.y = 0.0f;
            }
          break;

          case Gnome.BGPlacement.SPANNED:
            {
              this.bg_image.set_repeat (false, false);
              this.bg_image.set_sync_size (true);
              this.bg_image.set_keep_aspect_ratio (true);
              this.x = 0.0f;
              this.y = 0.0f;
            }
          break;
        }
    }

    private void
    _on_allocation_changed ()
    {
      Timeout.add (0, () => {
        if (this.filename != "")
          _update_image ();
        else
          _update_gradient ();

          return false;
      });
    }

    private void
    _update_image ()
    {

      if (this.find_child_by_name ("bg_image") != this.bg_image)
        {
          if (this.bg_gradient is Clutter.Actor &&
              this.bg_gradient.get_parent () == this)
            {
              this.remove_actor (this.bg_gradient);
            }
          this.add_actor (this.bg_image);
          this.bg_image.show ();
        }

      try
        {
          this.bg_image.set_from_file (this.filename);
        }
      catch (Error e)
        {
          warning ("Background: Unable to load background file %s: %s",
                   this.filename,
                   e.message);
        }
    }

    private void
    _update_gradient ()
    {
      Gnome.BGColorType type;
      Gdk.Color         primary;
      Gdk.Color         secondary;

      if (this.find_child_by_name ("bg_gradient") != this.bg_gradient)
        {
          if (this.bg_image is Clutter.Actor &&
              this.bg_image.get_parent () == this)
            {
              this.remove_actor (this.bg_image);
            }
          this.add_actor (this.bg_gradient);
          this.bg_gradient.show ();
        }

      this.x = 0.0f;
      this.y = 0.0f;

      this.gbg.get_color (out type, out primary, out secondary);

      this.bg_gradient.set_surface_size ((uint) this.width,
                                         (uint) this.height);

      switch (type)
        {
          case Gnome.BGColorType.SOLID:
            {

              Cairo.Context cr = this.bg_gradient.create ();
              cr.set_operator (Cairo.Operator.OVER);
              cr.scale (1.0f, 1.0f);
              cr.set_source_rgb ((double) primary.red   / (double) 0xffff,
                                 (double) primary.green / (double) 0xffff,
                                 (double) primary.blue  / (double) 0xffff);
              cr.rectangle (0.0f,
                            0.0f,
                            (double) this.width,
                            (double) this.height);
              cr.fill ();
            }
          break;

          case Gnome.BGColorType.V_GRADIENT:
            {
              Cairo.Context cr  = this.bg_gradient.create ();
              Cairo.Pattern pat = new Cairo.Pattern.linear (0.0f,
                                                            0.0f,
                                                            0.0f,
                                                            (double) this.height);
              cr.set_operator (Cairo.Operator.OVER);
              cr.scale (1.0f, 1.0f);
              pat.add_color_stop_rgb (0.0f,
                                      (double) primary.red   / (double) 0xffff,
                                      (double) primary.green / (double) 0xffff,
                                      (double) primary.blue  / (double) 0xffff);
              pat.add_color_stop_rgb (1.0f,
                                      (double) secondary.red   / (double) 0xffff,
                                      (double) secondary.green / (double) 0xffff,
                                      (double) secondary.blue  / (double) 0xffff);
              cr.set_source (pat);
              cr.rectangle (0.0f,
                            0.0f,
                            (double) this.width,
                            (double) this.height);
              cr.fill ();
            }
          break;

          case Gnome.BGColorType.H_GRADIENT:
            {
              Cairo.Context cr  = this.bg_gradient.create ();
              Cairo.Pattern pat = new Cairo.Pattern.linear (0.0f,
                                                            0.0f,
                                                            (double) this.width,
                                                            0.0f);
              cr.set_operator (Cairo.Operator.OVER);
              cr.scale (1.0f, 1.0f);
              pat.add_color_stop_rgb (0.0f,
                                      (double) primary.red   / (double) 0xffff,
                                      (double) primary.green / (double) 0xffff,
                                      (double) primary.blue  / (double) 0xffff);
              pat.add_color_stop_rgb (1.0f,
                                      (double) secondary.red   / (double) 0xffff,
                                      (double) secondary.green / (double) 0xffff,
                                      (double) secondary.blue  / (double) 0xffff);
              cr.set_source (pat);
              cr.rectangle (0.0f,
                            0.0f,
                            (double) this.width,
                            (double) this.height);
              cr.fill ();
            }
          break;
        }
    }

    private void
    _on_gconf_changed (GConf.Client client,
                       uint         cxnid,
                       GConf.Entry  entry)
    {
      bool   needs_update = false;
      string new_value;

      this.gbg.load_from_preferences (client);

      try
        {
          new_value = this.client.get_string (this.BG_PIC_OPTS);
        }
      catch (Error e)
        {
          new_value = DEFAULT_PIC_OPTS;
        }
      if (new_value != this.pic_opts)
      {
        this.pic_opts = new_value;
        needs_update = true;
      }

      try
        {
          new_value = this.client.get_string (this.BG_SHADE_TYPE);
        }
      catch (Error e)
        {
          new_value = DEFAULT_SHADE_TYPE;
        }
      if (new_value != this.shade_type)
      {
        this.shade_type = new_value;
        needs_update = true;
      }

      try
        {
          new_value = this.client.get_string (this.BG_PRIM_COL);
        }
      catch (Error e)
        {
          new_value = DEFAULT_AMBIANCE_COL;
        }
      if (new_value != this.prim_col)
      {
        this.prim_col = new_value;
        needs_update = true;
      }

      try
        {
          new_value = this.client.get_string (this.BG_SEC_COL);
        }
      catch (Error e)
        {
          new_value = DEFAULT_AMBIANCE_COL;
        }
      if (new_value != this.sec_col)
        {
          this.sec_col = new_value;
          needs_update = true;
        }

      try
        {
          new_value = this.client.get_string (this.BG_FILENAME);
        }
      catch (Error e)
       {
         new_value = DEFAULT_FILENAME;
       }
      if (new_value != this.filename)
      {
        this.filename = new_value;
        needs_update = true;
      }

      if (needs_update)
        {
          if (this.filename != "")
            _update_image ();
          else
            _update_gradient ();
        }
    }
  }
}
