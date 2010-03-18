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

    public UnityIcon (Clutter.Texture? icon, Clutter.Texture? bg_tex)
    {
      Object (icon: icon, bg_color: bg_tex);
    }

    construct
    {
      if (!(unity_icon_bg_layer is Clutter.Texture))
        {
          unity_icon_bg_layer = new ThemeImage ("prism_icon_background");
          unity_icon_fg_layer = new ThemeImage ("prism_icon_foreground");
          unity_icon_mk_layer = new ThemeImage ("prism_icon_mask");


        }

      if (this.icon is Clutter.Texture)
        {
          this.icon.set_parent (this);
          var icon_mat = new Cogl.Material ();
          Cogl.Texture icon_tex = (Cogl.Texture)(this.icon.get_cogl_texture ());
          Cogl.Texture mask_tex = (Cogl.Texture)(unity_icon_mk_layer.get_cogl_texture ());
          icon_mat.set_layer (0, icon_tex);
          icon_mat.set_layer (1, mask_tex);
          this.icon_material = icon_mat;
        }
      if (this.bg_color is Clutter.Texture)
        {
          this.bg_color.set_parent (this);
          this.bgcol_material = new Cogl.Material ();
          Cogl.Texture color = (Cogl.Texture)(this.bg_color.get_cogl_texture ());
          Cogl.Texture mask_tex = (Cogl.Texture)(unity_icon_mk_layer.get_cogl_texture ());
          this.bgcol_material.set_layer (0, color);
          this.bgcol_material.set_layer_filters (1, Cogl.MaterialFilter.NEAREST, Cogl.MaterialFilter.NEAREST);
          this.bgcol_material.set_layer (1, mask_tex);
        }

        var mat = new Cogl.Material ();
        Cogl.Texture tex = (Cogl.Texture)(unity_icon_bg_layer.get_cogl_texture ());
        mat.set_layer (0, tex);
        this.bg_mat = mat;

        mat = new Cogl.Material ();
        tex = (Cogl.Texture)(unity_icon_fg_layer.get_cogl_texture ());
        mat.set_layer (0, tex);
        this.fg_mat = mat;
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
      if (this.icon is Clutter.Texture)
        this.icon.allocate (box, flags);
      if (this.bg_color is Clutter.Texture)
        this.bg_color.allocate (box, flags);
    }

    public override void pick (Clutter.Color color)
    {
      this.set_effects_painting (true);
      var mat = new Cogl.Material ();
      mat.set_color4ub (color.red, color.green, color.blue, color.alpha);
      Cogl.rectangle (0, 0, 48, 48);
      base.pick (color);
      this.set_effects_painting (false);
    }

    /* The closest most horrible thing you will ever see in vala. its basically
     * C code... - oh well it works
     */
    public static void paint_real (Clutter.Actor actor)
    {
      UnityIcon self = actor as UnityIcon;

      Clutter.ActorBox box = Clutter.ActorBox ();
      self.get_stored_allocation (out box);

      /* we draw everything with cogl because Clutter.Texture seems to be made
       * of dumb. also it likes to double allocate everything
       */
      Cogl.set_source (self.bg_mat);
      Cogl.rectangle (box.x1, box.y1, box.x2, box.y2);

      if (self.bg_color is Clutter.Texture)
        {
          Cogl.set_source (self.bgcol_material);
          Cogl.rectangle (box.x1, box.y1, box.x2, box.y2);
        }
      if (self.icon is Clutter.Texture)
        {
          Cogl.set_source (self.icon_material);
          Cogl.rectangle (box.x1, box.y1, box.x2, box.y2);
        }

      Cogl.set_source (self.fg_mat);
      Cogl.rectangle (box.x1, box.y1, box.x2, box.y2);
    }

    public override void paint ()
    {
      /* we need a beter way of doing this effects stuff in vala, its horrible
       * to do, must have a think...
       */
      unowned SList<Ctk.Effect> effects = this.get_effects ();
      if (!this.get_effects_painting () && effects != null)
        {
          unowned SList<Ctk.Effect> e;
          this.set_effects_painting (true);
          for (e = effects; e != null; e = e.next)
            {
              Ctk.Effect effect = e.data;
              bool last_effect = (e.next != null) ? false : true;
              effect.paint (this.paint_real, last_effect);
            }

          this.set_effects_painting (false);
        }
      else
        {
          this.paint_real (this);
        }
    }

    public override void map ()
    {
      base.map ();
      this.icon.map ();
    }

    public override void unmap ()
    {
      base.map ();
      this.icon.unmap ();
    }
  }
}
