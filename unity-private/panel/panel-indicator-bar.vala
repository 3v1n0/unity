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
      Unity.Testing.ObjectRegistry.get_default ().register ("IndicatorBar", this);
      
      indicator_array = new Gee.ArrayList<Indicators.IndicatorObjectView> ();

      var model = IndicatorsModel.get_default ();
      var indicators_list = model.get_indicators ();

      foreach (Indicator.Object o in indicators_list)
        {
          var name = model.get_indicator_name (o);

          if (name != "libappmenu.so")
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
      int pos = this.indicator_array.index_of (object_view);
      if (pos == -1)
        return;
  
      if (type == Gtk.MenuDirectionType.PARENT)
        {
          if (pos == 0)
            {
              pos = this.indicator_array.size - 1;
            }
          else
            pos -= 1;
        }
      else if (type == Gtk.MenuDirectionType.CHILD)
        {
          if (pos == this.indicator_array.size - 1)
            {
              pos = 0;
            }
          else
            pos +=1;
        }

      IndicatorObjectView next_object_view = this.indicator_array.get (pos);
      
      if (type == Gtk.MenuDirectionType.PARENT)
        {
          next_object_view.open_last_menu_entry ();
        }
        
      if (type == Gtk.MenuDirectionType.CHILD)
        {
          next_object_view.open_first_menu_entry ();
        }       
    }
    
    public IndicatorObjectView? get_indicator_view_matching (Indicator.Object o)
    {
      foreach (Indicators.IndicatorObjectView i in indicator_array)
        {
          if(i.indicator_object == o)
            return i;
        }
      return null;
    }
    
    public void set_indicator_mode (bool mode)
    {
    }
  }
}
