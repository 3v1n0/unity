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
    private Clutter.CairoTexture bg;
    private Gnome.BG             gbg;

    construct
    {
      START_FUNCTION ();

      this.gbg = new Gnome.BG ();
      this.gbg.changed.connect (this._on_changed);
      this.gbg.transitioned.connect (this._on_transitioned);

      print ("\n---=== construct called ===---\n\n");

      this.bg = new Clutter.CairoTexture (1, 1);
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

      END_FUNCTION ();
    }

    private void
    _update_gradients ()
    {
      Gnome.BG          gbg = new Gnome.BG ();
      Gnome.BGColorType type;
      Gdk.Color         primary;
      Gdk.Color         secondary;

      print ("_update_gradient() called\n");

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

    private void
    _on_changed ()
    {
      print ("\n_on_changed() called ===---\n\n");
    }

    private void
    _on_transitioned ()
    {
      print ("\n_on_transitioned() called ===---\n\n");
    }
  }
}
