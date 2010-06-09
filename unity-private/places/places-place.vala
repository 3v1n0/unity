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

using GLib;
using Gee;

namespace Unity.Places
{
  /**
   * Represents a Place through a .place file ("offline") and then through
   * DBus ("online").
   **/
  public class Place : Object
  {
    const string PLACE_GROUP = "Place";
    const string ENTRY_PREFIX = "Entry:";

    /* Properties */
    public string dbus_name { get; construct; }
    public string dbus_path { get; construct; }

    public int n_entries {
      get {
        return entries_array.size;
      }
    }

    /* Whether the Place is available on the bus, this is only when we want to
     * do optimisations for startup (showing the entries before actually
     * starting the daemon. We can also abuse this for testing :)
     */
    public bool   online    { get; set; }

    private ArrayList<PlaceEntry> entries_array;

    /* Signals */
    public signal void entry_added   (PlaceEntry entry);
    public signal void entry_removed (PlaceEntry entry);

    /* Constructors */
    public Place (string dbus_name, string dbus_path)
    {
      Object (dbus_name:dbus_name, dbus_path:dbus_path);
    }

    construct
    {
      online = false;
      entries_array = new ArrayList<PlaceEntry> ();
    }

    /* Public Methods */
    public static Place? new_from_keyfile (KeyFile file, string id = "Unknown")
    {
      string errmsg = @"Unable to load place '$id': %s";

      if (file.has_group (PLACE_GROUP) == false)
        {
          warning (errmsg,"Does not contain 'Place' group");
          return null;
        }

      try {
        var dbus_name = file.get_string (PLACE_GROUP, "DBusName");
        var dbus_path = file.get_string (PLACE_GROUP, "DBusPath");

        var place = new Place (dbus_name, dbus_path);
        place.load_keyfile_entries (file);

        return place;

      } catch  (Error e) {
          warning (errmsg, e.message);
          return null;
      }
    }

    public PlaceEntry? get_nth_entry (int index)
    {
      return entries_array.get (index);
    }

    /* Private Methods */
    private void load_keyfile_entries (KeyFile file)
    {
      /* We've got the basic Place, now to try and load in any entry information
       * that exists
       */
      var groups = file.get_groups ();
      foreach (string group in groups)
        {
          if (group.has_prefix (ENTRY_PREFIX))
            {
              PlaceEntry? entry = load_entry_from_keyfile (file, group);
              if (entry is PlaceEntry)
                {
                  entries_array.add (entry);
                  entry_added (entry);
                }
            }
        }
    }

    private PlaceEntry? load_entry_from_keyfile (KeyFile file, string group)
    {
      try {
          var path = file.get_string (group, "DBusObjectPath");

          /* As we use the path to match those entries from the online daemon
           * to the loaded state, we really need the path
           */
          if (path == null || path == "" || path[0] != '/')
            {
              warning (@"Cannot load entry '$group': Does not contain valid DBusObjectPath");
              return null;
            }

          var name = file.get_string (group, "Name");
          var icon = file.get_string (group, "Icon");
          var desc = file.get_string (group, "Description");
          var show_global = file.get_boolean (group, "ShowGlobal");
          var show_entry = file.get_boolean (group, "ShowEntry");

          return new PlaceEntry.with_info (dbus_name,
                                           path,
                                           name,
                                           icon,
                                           desc,
                                           show_global,
                                           show_entry);

      } catch (Error e) {
        warning (@"Cannot load entry '$group': %s", e.message);
        return null;
      }
    }
  }
}

