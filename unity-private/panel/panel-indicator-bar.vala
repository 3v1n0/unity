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

namespace Unity.Panel
{
  public class IndicatorBar : Ctk.HBox
  {
    private Gee.ArrayList<Indicators.IndicatorObjectView> indicator_array;
    
    public IndicatorBar ()
    {
      Object (orientation: Ctk.Orientation.HORIZONTAL,
              spacing: 4);
    }
    
    construct
    {
      stdout.printf ("--- IndicatorBar: Constructor\n");
      indicator_array = new Gee.ArrayList<Indicators.IndicatorObjectView> ();
      Gee.Map<string, Indicator.Object>  indicators_map = Indicators.IndicatorsModel.get_default ().get_indicators ();
      
      stdout.printf ("--- IndicatorBar: Creating Indicator View\n");
      foreach (string key in indicators_map.keys)
        {
          if(key != "appmenu.so")
            {
              stdout.printf ("--- IndicatorBar: Loading %s\n", key);
              Indicators.IndicatorObjectView indicator_object_view = new Indicators.IndicatorObjectView (indicators_map[key]);
              
              stdout.printf ("--- IndicatorBar: IndicatorObjectView created for %s\n", key);
              indicator_object_view.menu_moved.connect (this.on_menu_moved);
              this.indicator_array.add (indicator_object_view);
              
              this.add_actor (indicator_object_view);
              //indicator_object_view.set_parent (this);
              indicator_object_view.show ();
              
            }
        }

      // Create IndicatorObjectViews as necessary
      // Connect to menu_moved signals
      stdout.printf ("--- IndicatorBar: End Constructor\n");
    }
    
    private void on_menu_moved (Indicators.IndicatorObjectView object_view, Gtk.MenuDirectionType type)
    {
      // Todo: Open left or right indicator, or circule back to the first or last indicator
    }
  }
}