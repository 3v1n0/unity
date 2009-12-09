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
 * Authored by Mirco "MacSlow" Müller <mirco.mueller@canonical.com>
 *
 */

namespace Unity.Places.Bar
{
  public class View : Ctk.Box
  {
    private Gee.ArrayList<Unity.Places.Bar.Model> places;

    public View ()
    {
      Unity.Places.Bar.Model place;
      int                    i;
      Ctk.Image              icon;
      int                    icon_size = 64;

      this.homogeneous  = false;
      this.spacing      = icon_size/2;
      this.orientation  = Ctk.Orientation.HORIZONTAL; // this sucks

      this.places = new Gee.ArrayList<Unity.Places.Bar.Model> ();

      // populate places-bar with hard-coded contents for the moment
      place = new Unity.Places.Bar.Model ("Home",
                                          "folder-home",
                                          "Default View");
      this.places.add (place);

      place = new Unity.Places.Bar.Model ("Applications",
                                          "applications-games",
                                          "Programs installed locally");
      this.places.add (place);

      place = new Unity.Places.Bar.Model ("Files",
                                          "folder",
                                          "Your files stored locally");
      this.places.add (place);

      place = new Unity.Places.Bar.Model ("Trash",
                                          "trashcan_empty",
                                          "Your piece of waste");
      this.places.add (place);

      // create all image-actors for icons
      for (i = 0; i < this.places.size ; i++)
      {
        icon = new Ctk.Image.from_stock (icon_size, this.places[i].icon_name);
        this.pack (icon, false, false);
      }

      this.show_all ();
    }

    construct
    {
    }
  }
}
