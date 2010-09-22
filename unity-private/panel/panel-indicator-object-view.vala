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
      indicator_object.menu_show.connect (on_menu_show);

      unowned GLib.List<Indicator.ObjectEntry> list = indicator_object.get_entries ();

      for (int i = 0; i < list.length (); i++)
        {
          unowned Indicator.ObjectEntry indicator_object_entry = (Indicator.ObjectEntry) list.nth_data (i);

          IndicatorObjectEntryView object_entry_view = new IndicatorObjectEntryView (indicator_object_entry);

          object_entry_view.menu_moved.connect (this.on_menu_moved);
          object_entry_view.entry_shown.connect (on_entry_shown);

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

    public void on_menu_show (Indicator.ObjectEntry entry, uint timestamp)
    {
      foreach (IndicatorObjectEntryView view in indicator_entry_array)
        {
          var s = (view.entry.label is Gtk.Label) ? entry.label.label : "";
          if (view.entry == entry)
            {
              view.show_menu ();
              break;
            }
        }
    }

    private void on_menu_moved (IndicatorObjectEntryView object_entry_view, Gtk.MenuDirectionType type)
    {
      if (type != Gtk.MenuDirectionType.PARENT &&
          type != Gtk.MenuDirectionType.CHILD)
        return;

      int pos = this.indicator_entry_array.index_of (object_entry_view);
      if (pos == -1)
        return;
  
      if (type == Gtk.MenuDirectionType.PARENT)
        {
          if (pos == 0)
            {
              this.menu_moved (type);
              return;
            }
          pos -= 1;
        }
      else if (type == Gtk.MenuDirectionType.CHILD)
        {
          if (pos == this.indicator_entry_array.size - 1)
          {
            this.menu_moved (type);
            return;
          }
          pos +=1;
        }

      IndicatorObjectEntryView next_object_entry_view = this.indicator_entry_array.get (pos);
      if (next_object_entry_view.skip)
        {
          if (type == Gtk.MenuDirectionType.PARENT)
            {
              if (pos == 0)
                next_object_entry_view = this.indicator_entry_array.get (this.indicator_entry_array.size - 1);
              else
                next_object_entry_view = this.indicator_entry_array.get (pos-1);

            }
          else if (type == Gtk.MenuDirectionType.CHILD)
            {
              if (pos == this.indicator_entry_array.size - 1)
                next_object_entry_view = this.indicator_entry_array.get (0);
              else
                next_object_entry_view = this.indicator_entry_array.get (pos+1);
            }
        }

      next_object_entry_view.show_menu ();
      /* Signal to be picked up by IndicatorBar */
      //this.menu_moved (type);
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
      object_entry_view.entry_shown.connect (on_entry_shown);

      this.indicator_entry_array.add (object_entry_view);
      this.add_actor (object_entry_view);

    }

    private void on_entry_shown (IndicatorObjectEntryView view)
    {
      indicator_object.entry_activate (view.entry,
                                       global_shell.get_current_time ());
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

    public void open_first_menu_entry ()
    {
      if (indicator_entry_array.size > 0)
        {
          IndicatorObjectEntryView object_entry_view = this.indicator_entry_array.get (0);
          object_entry_view.show_menu ();
        }
    }
    
    public void open_last_menu_entry ()
    {
      if (indicator_entry_array.size > 0)
        {
          IndicatorObjectEntryView object_entry_view = this.indicator_entry_array.get (indicator_entry_array.size-1);
          object_entry_view.show_menu ();
        }      
    }
    
    public bool find_entry (Indicator.ObjectEntry entry)
    {
      for (int i = 0; i < indicator_entry_array.size; i++)
        {
          IndicatorObjectEntryView object_entry_view = this.indicator_entry_array.get (i) as IndicatorObjectEntryView;
          if (object_entry_view.entry == entry)
            {
              return true;
            }
        }
      return false;
    }
    
    public IndicatorObjectEntryView? get_entry_view (Indicator.ObjectEntry entry)
    {
      for (int i = 0; i < indicator_entry_array.size; i++)
        {
          IndicatorObjectEntryView object_entry_view = this.indicator_entry_array.get (i) as IndicatorObjectEntryView;
          if (object_entry_view.entry == entry)
            {
              return object_entry_view;
            }
        }
      return null;
    }
    
    
    
    /* Hack for testing purpose */
    public void remove_first_entry ()
    {
      if(indicator_entry_array.size == 0)
        return;
   
      this.remove_actor (indicator_entry_array.remove_at (0));
      //indicator_entry_array.remove_at (0);
    }
  }
}
