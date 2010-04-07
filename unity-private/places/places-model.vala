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
  public class Model : Object
  {
    /**
     * Contains a list of places
     **/

    /* Properties */
    public ArrayList<Place> list;

    /* Signals */
    public signal void place_added (Place place);
    public signal void place_removed (Place place);
    public signal void place_changed (Place place);

    public Model ()
    {
      Object ();
    }

    construct
    {
      list = new ArrayList<Place> ();
    }

    public void add (Place place)
    {
      this.list.add (place);

      this.place_added (place);
    }

    public void remove (Place place)
    {
      this.list.remove (place);

      this.place_removed (place);
    }

  }
}

