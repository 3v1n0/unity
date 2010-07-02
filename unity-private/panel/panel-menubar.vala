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
 * Authored by canonical.com
 *
 */

namespace Unity.Panel.Indicators
{
  public class MenuBar : Ctk.Box
  {
    public IndicatorObjectView indicator_object_view;

    public MenuBar ()
    {
      Object (spacing:0,
              homogeneous:false,
              orientation:Ctk.Orientation.HORIZONTAL);
    }

    construct
    {
      var model = IndicatorsModel.get_default ();
      var indicators_list = model.get_indicators ();

      foreach (Indicator.Object o in indicators_list)
        {
          var name = model.get_indicator_name (o);

          if (name ==  "libappmenu.so")
            {
              indicator_object_view = new IndicatorObjectView (o);
              indicator_object_view.menu_moved.connect (this.on_menu_moved);

              pack (indicator_object_view, false, true);
              indicator_object_view.show ();

              break;
            }
        }

      /* Now let's add a rect as a hill-billy expander */
      var rect = new Clutter.Rectangle.with_color ({255, 0, 0, 0});
      pack (rect, true, true);
      rect.show ();
    }

    private void on_menu_moved (IndicatorObjectView object_view, Gtk.MenuDirectionType type)
    {
      if (type == Gtk.MenuDirectionType.PARENT)
        {
          indicator_object_view.open_last_menu_entry ();
        }
        
      if (type == Gtk.MenuDirectionType.CHILD)
        {
          indicator_object_view.open_first_menu_entry ();
        }
    }
  }
}
