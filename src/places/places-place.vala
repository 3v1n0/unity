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
  public abstract class Place : Object
  {
    /**
     * Represents a Place (icon, view etc). Will eventually be able to be
     * constructed from a Unity.Place.Proxy as well as sub-classed
     **/

    /* Properties */
    public string     name      { get; construct; }
    public string     icon_name { get; construct; }
    public string     comment   { get; construct; }

    private bool      _active;
    public  bool      active
      {
        get { return _active; }
        set { if (_active != value)
                {
                  _active = value;
                  if (_active)
                    this.activated ();
                }
            }
      }

    /* Signals */
    public signal void activated ();

    public Place (string name, string icon_name)
    {
      Object (name:name, icon_name:icon_name);
    }

    construct
    {
      _active = false;
    }

    public abstract Clutter.Actor get_view ();
  }
}

