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

using Unity;

namespace Unity.Places
{
  public enum SectionStyle
  {
    BUTTONS,
    BREADCRUMB
  }

  public class PlaceSearchSectionsBar : Ctk.Box
  {
    static const int SPACING = 10;

    /* Properties */
    public SectionStyle _style = SectionStyle.BUTTONS;
    public SectionStyle style {
      get { return _style; }
      set {
        if (_style != value)
          {
            _style = value;
            do_queue_redraw ();
          }
      }
    }

    private PlaceEntry? active_entry = null;
    private Section?    active_section = null;
    private CairoCanvas bg;

    private unowned Dee.Model? sections_model = null;

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

      bg = new CairoCanvas (paint_bg);
      set_background (bg);
    }

    /*
     * Public Methods
     */
    public void set_active_section (uint section_id)
    {
      var list = get_children ();

      unowned Section? section = list.nth_data<unowned Clutter.Actor>(section_id) as Section;

      if (section is Section)
        on_section_clicked_real (section);
    }

    public void set_active_entry (PlaceEntry entry)
    {
      var children = get_children ();

      if (entry == active_entry)
        {
          if (sections_model == entry.sections_model)
            return;
        }

      if (active_entry is PlaceEntry)
        {
          var old_model = active_entry.sections_model;
          old_model.row_added.disconnect (on_section_added);
          old_model.row_changed.disconnect (on_section_changed);
          old_model.row_removed.disconnect (on_section_removed);

          active_section = null;
        }

      if (entry.hints != null)
        {
          if (entry.hints["UnitySectionStyle"] == "breadcrumb")
            style = SectionStyle.BREADCRUMB;
          else
            style = SectionStyle.BUTTONS;
        }
      else
        style = SectionStyle.BUTTONS;

      foreach (Clutter.Actor actor in children)
        {
          (actor as Section).dirty = true;
        }


      active_entry = entry;
      var model = active_entry.sections_model;
      sections_model = model;

      unowned Dee.ModelIter iter = model.get_first_iter ();
      while (iter != null && !model.is_last (iter))
        {
          bool updated_local_entry = false;

          foreach (Clutter.Actor actor in children)
            {
              var s = actor as Section;

              if (s.dirty)
                {
                  s.model = model;
                  s.iter = iter;
                  s.text.text = model.get_string (iter, 0);
                  s.dirty = false;
                  updated_local_entry = true;

                  if (active_section == null)
                    active_section = s;

                  s.active = active_section == s;

                  break;
                }
            }
          if (!updated_local_entry)
            {
              on_section_added (model, iter);
            }

          iter = model.next (iter);
        }

      foreach (Clutter.Actor actor in children)
        {
          var s = actor as Section;
          if (s.dirty)
            {
              s.start_destroy ();
            }
        }

      model.row_added.connect (on_section_added);
      model.row_changed.connect (on_section_changed);
      model.row_removed.connect (on_section_removed);
    }

    private static int sort_sections (Section asec, Section bsec)
    {
      string op;

      var ret = asec.model.get_position (asec.iter) - bsec.model.get_position (bsec.iter);

      if (ret == 0)
        op = "=";
      else if (ret > 0)
        op = ">";
      else
        op = "<";

      print ("%s %s %s\n",
             asec.model.get_string (asec.iter, 0),
             op,
             bsec.model.get_string (bsec.iter, 0));
      
      return asec.model.get_position (asec.iter) - 
             bsec.model.get_position (bsec.iter);
    }

    private void on_section_added (Dee.Model model, Dee.ModelIter iter)
    {
      var section = new Section (model, iter);
      pack (section, false, true);
      section.show ();

      section.button_press_event.connect (on_section_clicked);

      if (active_section == null && _style == SectionStyle.BUTTONS)
        {
          active_section = section;
          section.active = true;
          active_entry.set_active_section (0);
        }
  
      sort_children ((CompareFunc)sort_sections);
    }

    private Section? get_section_for_iter (Dee.ModelIter iter)
    {
      var children = get_children ();
      foreach (Clutter.Actor child in children)
        {
          Section section = child as Section;
          if (section.iter == iter)
            return section;
        }

      return null;
    }

    private void on_section_changed (Dee.Model model, Dee.ModelIter iter)
    {
    }

    private void on_section_removed (Dee.Model model, Dee.ModelIter iter)
    {
      Section? section = get_section_for_iter (iter);

      if (section is Section)
        {
          if (section == active_section)
            {
              active_section = get_section_for_iter (model.get_first_iter ());
              active_entry.set_active_section (0);
            }
          section.start_destroy ();
        }
    }

    private void on_section_clicked_real (Section section)
    {
      active_section.active = false;
      active_section = section;
      active_section.active = true;
      var pos = active_section.model.get_position (active_section.iter);
      active_entry.set_active_section (pos);

    }

    private bool on_section_clicked (Clutter.Actor actor, Clutter.Event e)
    {
      Section section = actor as Section;

      if (section == active_section)
        return true;

      if (e.button.button != 1)
        return false;

      on_section_clicked_real (section);

      return true;
    }

    private void paint_bg (Cairo.Context cr, int width, int height)
    {
      if (_style != SectionStyle.BREADCRUMB)
        return;

      cr.set_operator (Cairo.Operator.CLEAR);
      cr.paint ();

      cr.set_operator (Cairo.Operator.OVER);
      cr.translate (0.5, 0.5);
      cr.set_line_width (1.0);

      var x = 0;
      var y = 0;
      width -= 1;
      height -= 1;
      var radius = 5;

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

      cr.set_source_rgba (1.0, 1.0, 1.0, 0.5);
      cr.fill_preserve ();
      cr.stroke ();

      var chevron = 5;
      var point = x;
      var children = get_children ();
      foreach (Clutter.Actor child in children)
        {
          point += (int)(child.width) + SPACING/2;

          if (point < width - chevron - SPACING)
            {
              cr.move_to (point - chevron, y);
              cr.line_to (point + chevron, y + height/2);
              cr.line_to (point - chevron, y + height);
              cr.set_source_rgba (1.0, 1.0, 1.0, 0.8);
              cr.stroke ();
            }
          point += SPACING/2;
        }
    }
  }

  private class Section : Ctk.Bin
  {
    static const float PADDING = 4.0f;

    private CairoCanvas bg;
    public  Ctk.Text    text;
    private Clutter.Color color;
    private Ctk.EffectGlow glow;

    public bool dirty = false;

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

    public unowned Dee.Model     model { get; construct set; }
    public unowned Dee.ModelIter iter { get; construct set; }

    public float destroy_factor {
        get { return _destroy_factor; }
        set { _destroy_factor = value; queue_relayout (); }
    }

    public float resize_factor {
        get { return _resize_factor; }
        set { _resize_factor = value; queue_relayout (); }
    }

    private float _destroy_factor = 1.0f;
    private float _resize_factor = 1.0f;
    private float _last_width = 0.0f;
    private float _resize_width = 0.0f;

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

    public void start_destroy ()
    {
      (get_parent () as Clutter.Container).remove_actor (this);
      /*
      if (_destroy_factor != 1.0f)
        return;

      var anim = animate (Clutter.AnimationMode.EASE_OUT_QUAD, 2000,
                          "destroy_factor", 0.0f);

      anim.completed.connect ((a) => {
        var obj = a.get_object () as Clutter.Actor;

        (obj.get_parent () as Clutter.Container).remove_actor (obj);
      });

      debug ("started_animation");
      */
    }

    private override void get_preferred_width (float     for_height,
                                               out float min_width,
                                               out float nat_width)
    {
     float mw, nw;

     base.get_preferred_width (for_height, out mw, out nw);

     min_width = (mw + padding.right + padding.left) * _destroy_factor;
     nat_width = (nw + padding.right + padding.left) * _destroy_factor;

     if (_last_width ==0.0f)
       _last_width = nat_width;

     if (_last_width != nat_width  && _resize_factor == 1.0)
       {
         _resize_factor = 0.0f;
         animate (Clutter.AnimationMode.EASE_OUT_QUAD, 100,
                  "resize_factor", 1.0f);

        _resize_width = _last_width;
        _last_width = nat_width;
       }

     if (_resize_factor != 1.0f)
       {
         min_width = _resize_width + ((min_width - _resize_width) * _resize_factor);
         nat_width = _resize_width + ((nat_width - _resize_width) * _resize_factor);
       }
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

      if ((get_parent () as PlaceSearchSectionsBar).style == SectionStyle.BUTTONS)
        {
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
      else
        {

        }
    }
  }
}
