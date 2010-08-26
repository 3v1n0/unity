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

    private Clutter.CairoTexture bg_texture;
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

      this.bg_texture = new Clutter.CairoTexture (1, 1);
      this.bg_texture.name = "bg_texture";

      this.gbg.load_from_preferences (client);

      // this is needed for the bg_texture to have the correct size
      this.allocation_changed.connect (_on_allocation_changed);

      END_FUNCTION ();
    }

    private void
    _on_allocation_changed ()
    {
      Timeout.add (0, () => {
        _update_gradient ();
        return false;
      });
    }

    private void
    _update_gradient ()
    {
      Gnome.BGColorType type;
      Gdk.Color         primary;
      Gdk.Color         secondary;

      if (this.find_child_by_name ("bg_texture") != this.bg_texture)
        {
          this.add_actor (this.bg_texture);
          this.bg_texture.show ();
        }

      this.x = 0.0f;
      this.y = 0.0f;

      this.gbg.get_color (out type, out primary, out secondary);

      this.bg_texture.set_surface_size ((uint) this.width,
                                         (uint) this.height);

      {
        Gdk.Pixbuf pixbuf = new Gdk.Pixbuf (Gdk.Colorspace.RGB,
                                            false,
                                            8,
                                            (int) this.width,
                                            (int) this.height);

        // the Gdk.Screen is not really used (because of passing false), it is
        // just passed to silence the vala-compiler
        this.gbg.draw (pixbuf, Gdk.Screen.get_default (), false);

        {
          Cairo.Context cr = this.bg_texture.create ();
          Gdk.cairo_set_source_pixbuf (cr, pixbuf, 0.0f, 0.0f);
          cr.paint ();
        }
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
          _update_gradient ();
        }
    }
  }
}
