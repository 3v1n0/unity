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
  public class View : Ctk.HBox
  {
    private Gee.ArrayList<Unity.Places.Bar.Model> places;

    public View ()
    {
      Unity.Places.Bar.Model place;

      this.places = new Gee.ArrayList<Unity.Places.Bar.Model> ();

      place = new Unity.Places.Bar.Model ("Home",
                                          "folder-home",
                                          "Default View");
      this.places.add (place);

      place = new Unity.Places.Bar.Model ("Applications",
                                          "applications-games",
                                          "Programs installed locally");
      this.places.add (place);

      place = new Unity.Places.Bar.Model ("Files",
                                          "files",
                                          "Your files stored locally");
      this.places.add (place);
    }

    construct
    {
    }
  }
}
