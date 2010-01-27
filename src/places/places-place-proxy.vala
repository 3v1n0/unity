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
    public string dbus_name { get; construct; }
    public string dbus_path { get; construct; }

    private DBus.Connection?     conn;
    private dynamic DBus.Object? service;

    /* Signals */

    public PlaceProxy (string name,
                       string icon_name,
                       string comment,
                       string dbus_name,
                       string dbus_path)
    {
      Object (name:name,
              icon_name:PKGDATADIR + "/applications.png",
              comment:comment,
              dbus_name:dbus_name,
              dbus_path:dbus_path);
    }

    construct
    {

    }

    private void setup_service ()
    {
      try
        {
          this.conn = DBus.Bus.get (DBus.BusType.SESSION);
          this.service = conn.get_object (this.dbus_name,
                                          this.dbus_path,
                                          "com.canonical.Unity.Place");

          this.service.ViewChanged += this.on_view_changed;

          this.service.set_active (false);
        }
      catch (Error e)
        {
          warning ("Unable to start service %s: %s",
                   this.dbus_name,
                   e.message);
        }
    }

    private void on_view_changed (dynamic DBus.Object       s,
                                  string                    view_name,
                                  HashTable<string, string> view_properties)
    {
      print (@"ViewChanged: $view_name, %d\n", view_properties.size ());

      print ("\t%s, %s\n",
            view_properties.lookup ("Hello"),
            view_properties.lookup ("Neil"));
    }

    public override Clutter.Actor get_view ()
    {
      /* Dump this in here for the moment */
      if (!(this.service is DBus.Object))
        {
          this.setup_service ();
        }

      return new Clutter.Rectangle ();
    }
  }
}

