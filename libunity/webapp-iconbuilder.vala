/*
 *      webapp-iconbuilder.vala
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
using Cairo;

namespace Unity.Webapp
{

  public class IconBuilder : Object
  {
    SvgSurface surf;
    Gdk.Pixbuf background;
    Gdk.Pixbuf foreground;
    ImageSurface mask;
    public string destination {get; construct;}
    public Gdk.Pixbuf source  {get; construct;}


    public IconBuilder (string dest, Gdk.Pixbuf source)
    {
      Object (destination: dest, source:source);
    }

    construct
    {
      this.surf = new SvgSurface (this.destination, 48, 48);
      this.load_layers ();
      if (this.source.get_width () > 48 || this.source.get_height () > 48)
        {
          this.source = this.source.scale_simple (48, 48, Gdk.InterpType.HYPER);
        }
    }

    public void load_layers ()
    {
      try {
        this.background = new Gdk.Pixbuf.from_file_at_scale ("/usr/share/unity/themes/prism_icon_background.png", 48, 48, true);
        this.foreground = new Gdk.Pixbuf.from_file_at_scale ("/usr/share/unity/themes/prism_icon_foreground.png", 48, 48, true);
      } catch (Error e) {
        warning (e.message);
      }

      this.mask = new Cairo.ImageSurface.from_png ("/usr/share/unity/themes/prism_icon_mask.png");
    }

    public void build_icon ()
    {
      debug (@"building icon $destination");
      Context cr = new Cairo.Context (this.surf);
      Gdk.cairo_set_source_pixbuf (cr, this.background, 0, 0);
      cr.paint ();
      cr.stroke ();

      float pad_left = 0;
      float pad_top = 0;
      if (this.source.get_width () < 48)
        {
          pad_left = (48 - this.source.get_width ()) / 2.0f;
        }
      if (this.source.get_height () < 48)
        {
          pad_top = (48 - this.source.get_height ()) / 2.0f;
        }
      Gdk.cairo_set_source_pixbuf (cr, this.source, pad_left, pad_top);
      cr.mask_surface (this.mask, 0, 0);

      Gdk.cairo_set_source_pixbuf (cr, this.foreground, 0, 0);
      cr.paint ();
      cr.stroke ();
    }
  }
}
