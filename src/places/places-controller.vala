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

namespace Unity.Places
{
  public class Controller : Object
  {
    /**
     * This class takes care of reading in the places, creating the view and
     * keeping it up-to-date
     **/
    private Model model;
    private View view;

    public Controller ()
    {
    }

    construct
    {
      this.model = new Model ();
      this.view = new View (this.model);

      Idle.add (this.load_places);
    }

    private bool load_places ()
    {
      /* Currently we have a static list, in the future we'll be using the
       * utility functions in libunity-places to load in the actual places
       * information from the drive
       */
      var place = new Place ("Home", PKGDATADIR + "/home.png");
      place.activated.connect (this.on_place_activated);
      this.model.add (place);

      place = new Place ("Applications", PKGDATADIR + "/applications.png");
      place.activated.connect (this.on_place_activated);
      this.model.add (place);

      place = new Place ("Files & Folders", PKGDATADIR + "/files.png");
      place.activated.connect (this.on_place_activated);
      this.model.add (place);

      return false;
    }

    private void on_place_activated (Place place)
    {
      debug ("Place activated: %s", place.name);
    }

    /* Public Methods */
    public View get_view ()
    {
      return this.view;
    }
  }
}

