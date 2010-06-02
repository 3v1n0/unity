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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 *              Jay Taoko <jay.taoko@canonical.com>
 *
 */

namespace Unity.Panel.Indicators
{
  public class IndicatorBar : Ctk.Box
  {
    private Gee.ArrayList<Indicators.IndicatorObjectView> indicator_array;

    public IndicatorBar ()
    {
      Object (orientation: Ctk.Orientation.HORIZONTAL,
              spacing: 0,
              homogeneous:false);
    }

    construct
    {
      indicator_array = new Gee.ArrayList<Indicators.IndicatorObjectView> ();

      var model = IndicatorsModel.get_default ();
      var indicators_list = model.get_indicators ();

      foreach (Indicator.Object o in indicators_list)
        {
          var name = model.get_indicator_name (o);

          if (name !=  "appmenu.so")
            {
              IndicatorObjectView indicator_object_view = new IndicatorObjectView (o);
              indicator_object_view.menu_moved.connect (this.on_menu_moved);
              this.indicator_array.add (indicator_object_view);

              this.add_actor (indicator_object_view);
              indicator_object_view.show ();

            }
        }

      // Create IndicatorObjectViews as necessary
      // Connect to menu_moved signals
    }

    private void on_menu_moved (IndicatorObjectView   object_view,
                                Gtk.MenuDirectionType type)
    {
      // Todo: Open left or right indicator, or circule back to the first or last indicator
    }
  }
}
