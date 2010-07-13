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
 */

namespace Unity.Testing
{
  public class Background : Ctk.Bin
  {
    /* Gnome Desktop background gconf keys */
    private string BG_DIR    = "/desktop/gnome/background";
    private string BG_FILE   = "/desktop/gnome/background/picture_filename";
    private string BG_OPTION = "/desktop/gnome/background/picture_options";
    private string BG_SHADING  = "/desktop/gnome/background/color_shading_type";
    private string BG_PRIM_COL = "/desktop/gnome/background/primary_color";
    private string BG_SEC_COL  = "/desktop/gnome/background/secondary_color";

    private string filename;
    private string option;

    private Clutter.CairoTexture bg;

    construct
    {
      START_FUNCTION ();
      var client = GConf.Client.get_default ();

      /* Setup the initial properties and notifies */
      /*try
        {
          client.add_dir (this.BG_DIR, GConf.ClientPreloadType.NONE);
        }
      catch (GLib.Error e)
        {
          warning ("Background: Unable to monitor background: %s", e.message);
        }

      try
        {
          this.filename = client.get_string (this.BG_FILE);
        }
      catch (Error e)
        {
          this.filename = "/usr/share/backgrounds/warty-final.png";
        }

      try
        {
          client.notify_add (this.BG_FILE, this.on_filename_changed);
        }
      catch (Error e)
        {
          warning ("Background: Unable to monitor background filename: %s",
                   e.message);
        }
      try
        {
          this.option = client.get_string (this.BG_OPTION);
        }
      catch (Error e)
        {
          this.option = "wallpaper";
        }
      try
        {
          client.notify_add (this.BG_OPTION, this.on_option_changed);
        }
      catch (Error e)
        {
          warning ("Background: Unable to monitor background options: %s",
                   e.message);
        }*/

      try
        {
          client.add_dir (this.BG_DIR, GConf.ClientPreloadType.NONE);
        }
      catch (GLib.Error e)
        {
          warning ("Background: Unable to monitor background: %s", e.message);
        }

      try
        {
          client.notify_add (this.BG_SHADING, this._update_gradients);
        }
      catch (Error e)
        {
          warning ("Background: Unable to monitor shading-type: %s", e.message);
        }

      try
        {
          client.notify_add (this.BG_PRIM_COL, this._update_gradients);
        }
      catch (Error e)
        {
          warning ("Background: Unable to monitor primary color: %s", e.message);
        }

      try
        {
          client.notify_add (this.BG_SEC_COL, this._update_gradients);
        }
      catch (Error e)
        {
          warning ("Background: Unable to monitor primary color: %s", e.message);
        }

      this.bg = new Clutter.CairoTexture (1, 1);

      //this.bg.set_load_async (true);
      this.add_actor (this.bg);
      this.bg.show ();

      /*if (filename == "" || filename == NULL)
      else
        {
          Gnome.BG gbg = new Gnome.BG ();

          public unowned Gdk.Pixmap gbg.create_pixmap (Gdk.Window window,
                                                       this.width,
                                                       this.height,
                                                       true);
          gdk_pixbuf_get_from_drawable (GdkPixbuf *dest,
                                        GdkDrawable *src,
                                        GdkColormap *cmap,
                                        int src_x,
                                        int src_y,
                                        int dest_x,
                                        int dest_y,
                                        int width,
                                        int height);
        }*/

      /* Load the texture */
      this.ensure_layout ();
      END_FUNCTION ();
    }

    private void
    _update_gradients ()
    {
      Gnome.BG          gbg = new Gnome.BG ();
      Gnome.BGColorType type;
      Gdk.Color         primary;
      Gdk.Color         secondary;

      gbg.get_color (out type, out primary, out secondary);

      this.bg.set_surface_size ((uint) this.width, (uint) this.height);

      switch (type)
        {
          case Gnome.BGColorType.SOLID:
            {

              Cairo.Context cr = this.bg.create ();
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
              cr.get_target().write_to_png ("/tmp/bg.png");
            }
          break;

          case Gnome.BGColorType.H_GRADIENT:
            {
              Cairo.Context cr  = this.bg.create ();
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
              cr.get_target().write_to_png ("/tmp/bg.png");
            }
          break;

          case Gnome.BGColorType.V_GRADIENT:
            {
              Cairo.Context cr  = this.bg.create ();
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
              cr.get_target().write_to_png ("/tmp/bg.png");
            }
          break;
        }
    }

    private void on_filename_changed (GConf.Client client,
                                      uint         cxnid,
                                      GConf.Entry  entry)
    {
      var new_filename = entry.get_value ().get_string ();

      if (new_filename == this.filename)
        return;

      this.filename = new_filename;
      this.ensure_layout ();
    }

    private void on_option_changed (GConf.Client client,
                                     uint         cxnid,
                                     GConf.Entry  entry)
    {
      var new_option = entry.get_value ().get_string ();

      if (new_option == this.option)
        return;

      this.option = new_option;
      this.ensure_layout ();
    }

    private void ensure_layout ()
    {
      try
        {
          this.bg.set_from_file (this.filename);
        }
      catch (Error e)
        {
          warning ("Background: Unable to load background file %s: %s",
                   this.filename,
                   e.message);
        }

      this.queue_relayout ();
    }
  }
}
