/*
 * Copyright (C) 2009 Canonical Ltd
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

using Gee;
using Unity.Launcher;
using Unity.Testing;

namespace Unity.Places
{
  public class Controller : Object
  {
    /**
     * This class takes care of reading in the places, creating the view and
     * keeping it up-to-date
     **/
    public  Shell shell { get; construct; }
    public  PlaceModel model { get; set; }

    private View view;

    public Controller (Shell shell)
    {
      Object (shell:shell);

      Testing.ObjectRegistry.get_default ().register ("UnityPlacesController", this);
    }

    construct
    {
      model = new PlaceFileModel () as PlaceModel;
      model.place_added.connect ((place) => {
        foreach (PlaceEntry e in place.get_entries ())
          on_entry_added (e);
      });

      view = new View (shell, model);
    }

    public View get_view ()
    {
      return view;
    }

    public void activate_entry (string entry_name)
    {
      foreach (Place place in model)
        {
          foreach (PlaceEntry entry in place.get_entries ())
            {
              if (entry.name == entry_name)
                {
                  view.on_entry_view_activated (entry, 0);
                  break;
                }
            }
        }
    }

    private void on_entry_added (PlaceEntry entry)
    {
      ScrollerModel s = ObjectRegistry.get_default ().lookup ("UnityScrollerModel")[0] as ScrollerModel;

      var child = new PlaceEntryScrollerChildController (entry);
      s.add (child.child);

      child.clicked.connect (on_entry_clicked);
    }

    private void on_entry_clicked (PlaceEntryScrollerChildController cont,
                                   uint                              section_id)
    {
     if (view.opacity == 0)
      {
        shell.show_unity ();
      }
     view.on_entry_view_activated (cont.entry, section_id);
    }
  }
}

