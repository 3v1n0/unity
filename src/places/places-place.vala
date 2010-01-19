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

namespace Unity.Places
{
  public class Place : Object
  {
    /**
     * Represents a Place (icon, view etc). Will eventually be able to be
     * constructed from a Unity.Place.Proxy as well as sub-classed
     **/

    /* Properties */
    public string     name      { get; construct; }
    public string     icon_name { get; construct; }
    public Gdk.Pixbuf icon      { get; construct; }
    public string     comment   { get; construct; }

    /* Signals */
    public signal void activated ();

    public Place (string name, string icon_name)
    {
      Object (name:name, icon_name:icon_name);
    }

    construct
    {
      try
        {
          this.icon = new Gdk.Pixbuf.from_file (this.icon_name);
        }
      catch (Error e)
        {
          warning (@"Unable to load '$icon_name' for '$name': %s", e.message);
        }
    }
  }
}

