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
    const string ACTIVATION_GROUP = "Activation";

    /* Properties */
    public string dbus_name { get; construct; }
    public string dbus_path { get; construct; }

    public int n_entries {
      get { return entries_array.size; }
    }

    /* Whether the Place is available on the bus, this is only when we want to
     * do optimisations for startup (showing the entries before actually
     * starting the daemon. We can also abuse this for testing :)
     */
    public bool   online    { get; set; }

    private DBus.Connection?     connection;
    private dynamic DBus.Object? service;
    private dynamic DBus.Object? activation_service;

    private ArrayList<PlaceEntry> entries_array;

    /* Activate API stuff */
    public Regex? uri_regex;
    public Regex? mime_regex;

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
        var dbus_path = file.get_string (PLACE_GROUP, "DBusObjectPath");

        var place = new Place (dbus_name, dbus_path);
        place.load_keyfile_entries (file);

        /* Activation hooks */
        try {
          var uri_match = file.get_string (ACTIVATION_GROUP, "URIPattern");
          if (uri_match != null && uri_match != "")
            {
              try {
                place.uri_regex = new Regex (uri_match);
              } catch (Error e) {
                warning (@"Unable to compile regex pattern $uri_match: $(e.message)");
              }
            }

          var mime_match = file.get_string (ACTIVATION_GROUP, "MimetypePattern");
          if (mime_match != null && mime_match != "")
            {
              try {
                place.mime_regex = new Regex (mime_match);
              } catch (Error e) {
                warning (@"Unable to compile regex pattern $mime_match: $(e.message)");
              }
            }
        } catch (Error e) {
          //fail silently, not all places will have activation hooks
        }

        return place;

      } catch  (Error e) {
          warning (errmsg, e.message);
          return null;
      }
    }

    public unowned ArrayList<PlaceEntry> get_entries ()
    {
      return entries_array;
    }

    public PlaceEntry? get_nth_entry (int index)
    {
      return entries_array.get (index);
    }

    /*
     * Connect to the place over DBus, refresh the entries, and finally ask
     * the updated list of entries to connect
     */
    public new void connect ()
    {
      if (online)
        return;

      /* Grab the place off DBus */
      try {
        connection = DBus.Bus.get (DBus.BusType.SESSION);
        service = connection.get_object (dbus_name,
                                         dbus_path,
                                         "com.canonical.Unity.Place");
      } catch (Error e) {
        warning (@"Unable to connect to $dbus_path on $dbus_name: %s",
                 e.message);
        return;
      }

      /* Connect to necessary signals */
      service.EntryAdded.connect (on_service_entry_added);
      service.EntryRemoved.connect (on_service_entry_removed);

      /* Make sure our entries are up-to-date */
      /* FIXME: In a world without Vala this would be async, however due to
       * it's troubles with marshalling complex types (which I concede cannot
       * be easy), we need to do a poor-mans async and call it in an idle
       * instead.
       */
      Idle.add (() => {
        ValueArray[] entries = service.get_entries ();
        for (int i = 0; i < entries.length; i++)
          {
            unowned ValueArray array = entries[i];
            var path = array.get_nth (0).get_string ();
            bool existing = false;

            foreach (PlaceEntry e in entries_array)
              {
                var entry = e as PlaceEntryDbus;
                if (entry.dbus_path == path)
                  {
                    entry.update_info (array);
                    entry.connect ();
                    entry.parent = this;
                    existing = true;
                  }
              }

            if (existing == false)
              {
                on_service_entry_added (service, array);
              }
          }

        /* Now remove those that couldn't connect or did not exist in the live
         * place
         */
        /* FIXME: Make this cleaner */
        ArrayList<PlaceEntry> old = new ArrayList<PlaceEntry> ();
        foreach (PlaceEntry entry in entries_array)
          {
            if (entry.online == false)
              old.add (entry);
          }
        foreach (PlaceEntry entry in old)
          {
            entry_removed (entry);
            entries_array.remove (entry);
          }
        return false;
      });

      online = true;
    }

    public bool activate (string uri)
    {
      if (activation_service == null)
        {
            activation_service = connection.get_object (dbus_name,
                                                        dbus_path,
                                                        "com.canonical.Unity.Activation");
        }

      return activation_service.activate (uri);
    }

    /* Private Methods */
    private void on_service_entry_added (dynamic DBus.Object   dbus_object,
                                         ValueArray            info)
    {
      debug ("EntryAdded %s", info.get_nth (0).get_string ());
      /* This is a new entry */
      var entry = new PlaceEntryDbus (dbus_name,
                                  info.get_nth (0).get_string ());
      entry.update_info (info);
      entries_array.add (entry);
      entry.connect ();
      entry_added (entry);
    }

    private void on_service_entry_removed (dynamic DBus.Object dbus_object,
                                           string              entry_path)
    {
      debug (@"EntryRemoved: $entry_path");
      PlaceEntry? entry = null;

      foreach (PlaceEntry ent in entries_array)
        {
          var e = ent as PlaceEntryDbus;
          if (e.dbus_path == entry_path)
            {
              entry = e;
              break;
            }
        }
      if (entry is PlaceEntry)
        {
          entry_removed (entry);
          entries_array.remove (entry);
        }
    }

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
      string path = "";
      string name = "";
      string icon = "";
      string desc = "";
      bool show_global = false;
      bool show_entry = false;

      try {
          path = file.get_string (group, "DBusObjectPath");

          /* As we use the path to match those entries from the online daemon
           * to the loaded state, we really need the path
           */
          if (path == null || path == "" || path[0] != '/')
            {
              warning (@"Cannot load entry '$group': Does not contain valid DBusObjectPath: $path");
              return null;
            }

          name = file.get_string (group, "Name");
          icon = file.get_string (group, "Icon");
          desc = file.get_string (group, "Description");
      } catch (Error e) {
        warning (@"Cannot load entry '$group': %s", e.message);
        return null;
      }

      try {
        show_global = file.get_boolean (group, "ShowGlobal");
        show_entry = file.get_boolean (group, "ShowEntry");
      } catch (Error e) {
          show_global = true;
          show_entry = true;
      }

      return new PlaceEntryDbus.with_info (dbus_name,
                                           path,
                                           name,
                                           icon,
                                           desc,
                                           show_global,
                                           show_entry);
    }
  }
}

