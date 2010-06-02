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
 */

using Gee;
using Utils;

namespace Unity.Panel.Indicators
{
  public class IndicatorObjectView : Ctk.Box
  {
    public Indicator.Object indicator_object { get; construct; }

    public signal void menu_moved (Gtk.MenuDirectionType type);

    private Gee.ArrayList<IndicatorObjectEntryView> indicator_entry_array;

    public IndicatorObjectView (Indicator.Object _object)
    {
      Object (indicator_object: _object,
              orientation:Ctk.Orientation.HORIZONTAL,
              spacing:0,
              homogeneous:false);
    }

    construct
    {
      START_FUNCTION ();

      /* Read through the entries in the object, creating an
       * IndicatorObjectEntryView for each one and appending it to self.
       * Connect to IndicatorObjectEntryView's menu_moved signal so we can show
       *  the previous or next menu when the user presses the left or right
       *  arrows on their keyboard.
       * If we're at the first or last entry, and the user want previous or
       *  next, when we emit the same signal, so our parent can pass it on to
       *  the IndicatorObjectView to the left of right of us.
       * Connect to the entry_added/removed/moved signals and do the right
       *  thing for them
       */
      indicator_entry_array = new Gee.ArrayList<IndicatorObjectEntryView> ();

      indicator_object.entry_added.connect (this.on_entry_added);
      indicator_object.entry_removed.connect (this.remove_entry);

      unowned GLib.List<Indicator.ObjectEntry> list = indicator_object.get_entries ();

      for (int i = 0; i < list.length (); i++)
        {
          unowned Indicator.ObjectEntry indicator_object_entry = (Indicator.ObjectEntry) list.nth_data (i);

          IndicatorObjectEntryView object_entry_view = new IndicatorObjectEntryView (indicator_object_entry);

          object_entry_view.menu_moved.connect (this.on_menu_moved);

          this.indicator_entry_array.add (object_entry_view);
          this.add_actor (object_entry_view);
        }

      END_FUNCTION ();
    }

    public void show_entry_menu (int entry)
    {
      /* Sometimes parent will want to force showing of an entry's menu,
       * for instance when the user is moving between menus using the arrow
       * keys.
       * TODO
       */
    }

    private void on_menu_moved (IndicatorObjectEntryView object_entry_view, Gtk.MenuDirectionType type)
    {
      /* Signal to be picked up by IndicatorBar */
      this.menu_moved (type);
    }

    private void on_entry_added (Indicator.Object object, Indicator.ObjectEntry indicator_object_entry)
    {
      /* First, make sure that this entry is not already in */
      for (int i = 0; i < indicator_entry_array.size; i++)
        {
          IndicatorObjectEntryView entry_view = this.indicator_entry_array.get (i) as IndicatorObjectEntryView;
          if (entry_view.entry == indicator_object_entry)
            {
              return;
            }
        }

      IndicatorObjectEntryView object_entry_view = new IndicatorObjectEntryView (indicator_object_entry);

      object_entry_view.menu_moved.connect (this.on_menu_moved);

      this.indicator_entry_array.add (object_entry_view);
      this.add_actor (object_entry_view);

    }

    private void remove_entry (Indicator.ObjectEntry entry)
    {
      for (int i = 0; i < indicator_entry_array.size; i++)
        {
          IndicatorObjectEntryView object_entry_view = this.indicator_entry_array.get (i) as IndicatorObjectEntryView;
          if (object_entry_view.entry == entry)
            {
              this.remove_actor (object_entry_view);
              this.indicator_entry_array.remove (object_entry_view);
            }
        }
    }
  }
}
