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
  public class MenuBar : Ctk.Bin
  {
    public MenuBar ()
    {
    }

    construct
    {
      var model = IndicatorsModel.get_default ();
      var indicators_list = model.get_indicators ();

      foreach (Indicator.Object o in indicators_list)
        {
          var name = model.get_indicator_name (o);

          if (name ==  "appmenu.so")
            {
              IndicatorObjectView indicator_object_view = new IndicatorObjectView (o);
              indicator_object_view.menu_moved.connect (this.on_menu_moved);


              this.add_actor (indicator_object_view);
              indicator_object_view.show ();
            }
        }
     }

    private void on_menu_moved (IndicatorObjectView   object_view,
                                Gtk.MenuDirectionType type)
    {
      // Todo: Open left or right indicator, or circule back to the first or last indicator
    }
  }
}
