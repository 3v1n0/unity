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
 * Authored by Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */

namespace Unity.Places
{
  public class PlaceProxy : Place
  {
    /**
     * Represents a Remote Place (icon, view etc).
     **/

    /* Properties */
    public string path { get; construct; }

    /* Signals */

    public PlaceProxy (string name,
                       string icon_name,
                       string comment,
                       string path)
    {
      Object (name:name,
              icon_name:PKGDATADIR + "/applications.png",
              comment:comment,
              path:path);
    }

    construct
    {

    }

    public override Clutter.Actor get_view ()
    {
      return new Clutter.Rectangle ();
    }
  }
}

