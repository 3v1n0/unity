/*
 *      icon-postprocessor.vala
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

namespace Unity
{
  /* just a layered actor that layers the different components of our
   * icon on top of each other
   */
  public class UnityIcon : Ctk.Actor
  {
    public Clutter.Texture? icon {get; construct;}
    public Clutter.Texture? bg_color {get; construct;}
    private Clutter.Texture bg_layer;
    private Clutter.Texture fg_layer;
    private Clutter.Texture mask;

    private Cogl.Material icon_material;
    private Cogl.Material bgcol_material;

    public UnityIcon (Clutter.Texture? icon, Clutter.Texture? bg_tex)
    {
      Object (icon: icon, bg_color: bg_tex);
    }

    construct
    {
      this.bg_layer = new ThemeImage ("prism_icon_background");
      this.fg_layer = new ThemeImage ("prism_icon_foreground");
      this.mask = new ThemeImage ("prism_icon_mask");

      this.bg_layer.set_parent (this);
      this.fg_layer.set_parent (this);
      this.mask.set_parent (this);

      if (this.icon is Clutter.Texture)
        {
          this.icon.set_parent (this);
          var icon_mat = new Cogl.Material ();
          Cogl.Texture icon_tex = (Cogl.Texture)(this.icon.get_cogl_texture ());
          Cogl.Texture mask_tex = (Cogl.Texture)(this.mask.get_cogl_texture ());
          icon_mat.set_layer (0, icon_tex);
          icon_mat.set_layer (1, mask_tex);
          this.icon_material = icon_mat;
        }
      if (this.bg_color is Clutter.Texture)
        {
          this.bg_color.set_parent (this);
          this.bgcol_material = new Cogl.Material ();
          Cogl.Texture color = (Cogl.Texture)(this.bg_color.get_cogl_texture ());
          Cogl.Texture mask_tex = (Cogl.Texture)(this.mask.get_cogl_texture ());
          this.bgcol_material.set_layer (0, color);
          this.bgcol_material.set_layer_filters (1, Cogl.MaterialFilter.NEAREST, Cogl.MaterialFilter.NEAREST);
          this.bgcol_material.set_layer (1, mask_tex);
        }
    }

    public override void get_preferred_width (float for_height,
                                              out float minimum_width,
                                              out float natural_width)
    {
      natural_width = minimum_width = 48;
    }

    public override void get_preferred_height (float for_width,
                                               out float minimum_height,
                                               out float natural_height)
    {
      natural_height = minimum_height = 48;
    }

    public override void allocate (Clutter.ActorBox box, Clutter.AllocationFlags flags)
    {
      base.allocate (box, flags);
      this.bg_layer.allocate (box, flags);
      this.fg_layer.allocate (box, flags);
      this.mask.allocate (box, flags);
      if (this.icon is Clutter.Texture)
        this.icon.allocate (box, flags);
      if (this.bg_color is Clutter.Texture)
        this.bg_color.allocate (box, flags);
    }

    public override void pick (Clutter.Color color)
    {
      base.pick (color);
      this.bg_layer.paint ();
    }

    public override void paint ()
    {
      base.paint ();
      this.bg_layer.paint ();

      Clutter.ActorBox box = Clutter.ActorBox ();
      this.get_stored_allocation (out box);
      if (this.bg_color is Clutter.Texture)
        {
          Cogl.set_source (this.bgcol_material);
          Cogl.rectangle (box.x1 + 2, box.y1, box.x2 + 2, box.y2);
        }
      if (this.icon is Clutter.Texture)
        {
          Cogl.set_source (this.icon_material);
          Cogl.rectangle (box.x1 +2, box.y1, box.x2 +2, box.y2);
        }
      this.fg_layer.paint ();
    }

    public override void map ()
    {
      base.map ();
      this.bg_layer.map ();
      this.icon.map ();
      this.fg_layer.map ();
    }

    public override void unmap ()
    {
      base.map ();
      this.bg_layer.unmap ();
      this.icon.unmap ();
      this.fg_layer.unmap ();
    }
  }
}
