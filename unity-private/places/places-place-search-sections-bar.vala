/*
 * Copyright (C) 2010 Canonical Ltd
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

namespace Unity.Places
{
  public class PlaceSearchSectionsBar : Ctk.Box
  {
    static const int SPACING = 10;

    /* Properties */
    private PlaceEntry? active_entry = null;
    private Section?    active_section = null;

    public PlaceSearchSectionsBar ()
    {
      Object (orientation:Ctk.Orientation.HORIZONTAL,
              homogeneous:false,
              spacing:SPACING);
    }

    construct
    {
      /*
      padding = {
        SPACING * 2.0f,
        SPACING * 1.0f,
        SPACING * 1.0f,
        SPACING * 1.0f
      };*/
    }

    /*
     * Public Methods
     */
    public void set_active_entry (PlaceEntry entry)
    {
      if (active_entry is PlaceEntry)
        {
          var old_model = active_entry.sections_model;
          old_model.row_added.disconnect (on_section_added);

          (this as Clutter.Container).foreach ((actor) => {
                                                   actor.destroy ();
                                                 },
                                               null);
          active_section = null;
        }

      active_entry = entry;
      var model = active_entry.sections_model;

      unowned Dee.ModelIter iter = model.get_first_iter ();
      while (iter != null && !model.is_last (iter))
        {
          on_section_added (model, iter);
          iter = model.next (iter);
        }

      model.row_added.connect (on_section_added);
    }

    private void on_section_added (Dee.Model model, Dee.ModelIter iter)
    {
      var section = new Section (model, iter);
      pack (section, false, true);
      section.show ();

      section.button_press_event.connect (on_section_clicked);

      if (active_section == null)
        {
          active_section = section;
          section.active = true;
          active_entry.set_active_section (0);
        }
    }

    private bool on_section_clicked (Clutter.Actor actor, Clutter.Event e)
    {
      Section section = actor as Section;

      if (section == active_section)
        return true;

      if (e.button.button != 1)
        return false;

      active_section.active = false;
      active_section = section;
      active_section.active = true;
      var pos = active_section.model.get_position (active_section.iter);
      active_entry.set_active_section (pos);

      return true;
    }
  }

  private class Section : Ctk.Bin
  {
    static const float PADDING = 4.0f;

    private CairoCanvas bg;
    private Ctk.Text    text;
    private Clutter.Color color;
    private Ctk.EffectGlow glow;

    public new string name {
      get { return text.text; }
      set {
        text.text = value;
        glow.set_invalidate_effect_cache (true);
      }
    }

    private bool _active = false;
    public bool active {
      get { return _active; }
      set {
        if (_active != value)
          {
            _active = value;

            Clutter.AnimationMode mode = Clutter.AnimationMode.EASE_OUT_QUAD;
            int opacity = 0;
            color = { 255, 255, 255, 255 };

            if (_active)
              {
                color = { 152, 74, 131, 255 };
                mode = Clutter.AnimationMode.EASE_IN_QUAD;
                opacity = 255;
              }

            text.color = color;
            bg.texture.animate (mode, 100, "opacity", opacity);
          }
      }
    }

    public unowned Dee.Model     model { get; construct; }
    public unowned Dee.ModelIter iter { get; construct; }

    public Section (Dee.Model model, Dee.ModelIter iter)
    {
      Object (reactive:true,
              model:model,
              iter:iter);
    }

    construct
    {
      color.alpha = 255;

      padding = { PADDING, PADDING, PADDING, PADDING };

      bg = new CairoCanvas (paint_bg);
      set_background (bg);
      bg.texture.opacity = 0;
      bg.show ();

      glow = new Ctk.EffectGlow ();
      glow.set_color ({ 255, 255, 255, 255 });
      glow.set_factor (1.0f);
      glow.set_margin (5);
      //add_effect (glow);

      text = new Ctk.Text (model.get_string (iter, 0));
      add_actor (text);
      text.show ();
    }

    private override void get_preferred_width (float     for_height,
                                               out float min_width,
                                               out float nat_width)
    {
     float mw, nw;

     base.get_preferred_width (for_height, out mw, out nw);

     min_width = mw + padding.right + padding.left;
     nat_width = nw + padding.right + padding.left;
    }

    private void paint_bg (Cairo.Context cr, int width, int height)
    {
      cr.set_operator (Cairo.Operator.CLEAR);
      cr.paint ();

      cr.set_operator (Cairo.Operator.OVER);
      cr.set_line_width (1.5);
      cr.set_source_rgba (1.0, 1.0, 1.0, 0.0);

      cr.translate (0.5, 0.5);

      var x = 0;
      var y = 0;
      width -= 1;
      height -= 1;
      var radius = 10;

      cr.line_to  (x, y + radius);
      cr.curve_to (x, y,
                   x, y,
                   x + radius, y);
      cr.line_to  (width - radius, y);
      cr.curve_to (width, y,
                   width, y,
                   width, y + radius);
      cr.line_to  (width, height - radius);
      cr.curve_to (width, height,
                   width, height,
                   width - radius, height);
      cr.line_to  (x + radius, height);
      cr.curve_to (x, height,
                   x, height,
                   x, height - radius);
      cr.close_path ();

      cr.set_source_rgba (1.0, 1.0, 1.0, 1.0);
      cr.fill_preserve ();
      cr.stroke ();
    }
  }
}
