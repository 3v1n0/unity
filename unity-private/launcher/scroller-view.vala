/* -*- Mode: vala; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
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
 * Authored by Gordon Allott <gord.allott@canonical.com>
 *
 */

namespace Unity.Launcher
{
  class ScrollerView : Ctk.Actor
  {
    public ScrollerModel model {get; set;}
    public int spacing = 5;

    public ScrollerView (ScrollerModel model)
    {
      Object (model:model);
    }

    construct
    {
      model = new ScrollerModel ();
    }

    /* Clutter overrides */
    public override void get_preferred_width (float for_height,
                                              out float minimum_width,
                                              out float natural_width)
    {
      minimum_width = 0;
      natural_width = 0;

      float pmin_width = 0.0f;
      float pnat_width = 0.0f;

      foreach (ScrollerChild child in model)
      {
        float cmin_width = 0.0f;
        float cnat_width = 0.0f;

        child.get_preferred_width (for_height,
                                   out cmin_width,
                                   out cnat_width);

        pmin_width = pmin_width.max(pmin_width, cmin_width);
        pnat_width = pnat_width.max(pnat_width, cnat_width);

      }

      pmin_width += padding.left + padding.right;
      pnat_width += padding.left + padding.right;

      minimum_width = pmin_width;
      natural_width = pnat_width;

    }

    public override void get_preferred_height (float for_width,
                                      out float minimum_height,
                                      out float natural_height)
    {
      minimum_height = 0.0f;
      natural_height = 0.0f;

      float cnat_height = 0.0f;
      float cmin_height = 0.0f;
      float all_child_height = 0.0f;
      foreach (ScrollerChild child in model)
      {
        cnat_height = 0.0f;
        cmin_height = 0.0f;
        child.get_preferred_height (for_width,
                                    out cmin_height,
                                    out cnat_height);
        all_child_height += cnat_height + this.spacing;
      }

      minimum_height = all_child_height + padding.top + padding.bottom;
      natural_height = all_child_height + padding.top + padding.bottom;
      return;
    }

    public override void allocate (Clutter.ActorBox box,
                                   Clutter.AllocationFlags flags)
    {
      float y = padding.top;

      foreach (ScrollerChild child in model)
        {
          Clutter.ActorBox child_box = Clutter.ActorBox ();
          child_box.x1 = 0;
          child_box.x2 = box.get_width ();
          child_box.y1 = y;
          child_box.y2 = box.get_height ();

          child.allocate (child_box, flags);
        }
    }

    public override void pick (Clutter.Color color)
    {
      base.pick (color);
      foreach (ScrollerChild child in model)
        {
          child.paint ();
        }
    }


    public override void paint ()
    {
      foreach (ScrollerChild child in model)
        {
          child.paint ();
        }
    }
  }

}
