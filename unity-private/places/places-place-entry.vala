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
  /**
   * Represents a PlaceEntry through a .place file ("offline") and then through
   * DBus ("online").
   **/
  public class PlaceEntry : Object
  {
    /* FIXME: Use what's in libunity. We only need this so vala allows us to
     * connect to the signal
     */
    public struct RendererInfo
    {
      string default_renderer;
      string groups_model;
      string results_model;
      HashTable<string, string> renderer_hints;
    }

    /*
     * Properties
     */
    public string? dbus_name   { get; construct; }
    public string? dbus_path   { get; construct; }
    public string  name        { get; construct set; }
    public string  icon        { get; construct set; }
    public string  description { get; construct set; }

    public uint     position  { get; set; }
    public string[] mimetypes { get; set; }
    public bool     sensitive { get; set; }

    public Gee.HashMap<string, string> hints { get; set; }

    /* Whether the Entry is available on the bus, this is only when we want to
     * do optimisations for startup (showing the entries before actually
     * starting the daemon. We can also abuse this for testing :)
     */
    public bool   online    { get; construct set; }

    /* This is loaded from the desktop file, isn't useful when the entry
     * is online
     */
    public bool    show_global { get; construct set; }
    public bool    show_entry  { get; construct set; }

    /* This is relayed to the service, in case it needs to do something special
     * when it's activated
     */
    private bool _active = false;
    public bool active {
      get { return _active; }
      set {
        if (_active != value)
          {
            _active = value;
            service.set_active (_active);
          }
      }
    }

    /* We store the name of the sections model, and we provide the actual
     * model too. The reason for this is that for testing, we can easily inject
     * a fake model into a fake entry and test the views. It also keeps things
     * cleaner for the views (they aren't creating their own models).
     */
    public string      sections_model_name { get; set; }
    private Dee.Model? _sections_model = null;
    public Dee.Model?  sections_model {
      get {
        if (_sections_model is Dee.Model == false)
          {
            if (sections_model_name != null)
              {
                _sections_model = new Dee.SharedModel.with_name (sections_model_name);
                (_sections_model as Dee.SharedModel).connect ();
              }
          }
        return _sections_model;
      }
      set {
        _sections_model = value;
      }
    }

    /* Entry rendering info */
    public string entry_renderer_name;

    public string      entry_groups_model_name;
    private Dee.Model? _entry_groups_model;
    public Dee.Model?  entry_groups_model {
      get {
        if (_entry_groups_model is Dee.Model == false)
          {
            if (entry_groups_model_name != null)
              {
                _entry_groups_model = new Dee.SharedModel.with_name (entry_groups_model_name);
                (_entry_groups_model as Dee.SharedModel).connect ();
              }
          }
        return _entry_groups_model;
      }
      set {
          _entry_groups_model = value;
      }
    }

    public string      entry_results_model_name;
    private Dee.Model? _entry_results_model;
    public Dee.Model?  entry_results_model {
      get {
        if (_entry_results_model is Dee.Model == false)
          {
            if (entry_results_model_name != null)
              {
                _entry_results_model = new Dee.SharedModel.with_name (entry_results_model_name);
                (_entry_results_model as Dee.SharedModel).connect ();
              }
          }
        return _entry_results_model;
      }
      set {
          _entry_results_model = value;
      }
    }

    public Gee.HashMap<string, string>? entry_renderer_hints;

    /* Global rendering info */
    public string global_renderer_name;

    public string      global_groups_model_name;
    private Dee.Model? _global_groups_model;
    public Dee.Model?  global_groups_model {
      get {
        if (_global_groups_model is Dee.Model == false)
          {
            if (global_groups_model_name != null)
              {
                _global_groups_model = new Dee.SharedModel.with_name (global_groups_model_name);
                (_global_groups_model as Dee.SharedModel).connect ();
              }
          }
        return _global_groups_model;
      }
      set {
          _global_groups_model = value;
      }
    }

    public string      global_results_model_name;
    private Dee.Model? _global_results_model;
    public Dee.Model?  global_results_model {
      get {
        if (_global_results_model is Dee.Model == false)
          {
            if (global_results_model_name != null)
              {
                _global_results_model = new Dee.SharedModel.with_name (global_results_model_name);
                (_global_results_model as Dee.SharedModel).connect ();
              }
          }
        return _global_results_model;
      }
      set {
          _global_results_model = value;
      }
    }

    public Gee.HashMap<string, string>? global_renderer_hints;

    /* Our connection to the place-entry over dbus */
    private DBus.Connection?     connection;
    private dynamic DBus.Object? service;

    /*
     * Signals
     */
    public signal void updated ();
    public signal void renderer_info_changed ();

    /*
     * Constructors
     */
    public PlaceEntry (string dbus_name, string dbus_path)
    {
      Object (dbus_name:dbus_name, dbus_path:dbus_path);
    }

    public PlaceEntry.with_info (string dbus_name,
                                 string dbus_path,
                                 string name,
                                 string icon,
                                 string description,
                                 bool   show_global,
                                 bool   show_entry)
    {
      Object (dbus_name:dbus_name,
              dbus_path:dbus_path,
              name:name,
              icon:icon,
              description:description,
              show_global:show_global,
              show_entry:show_entry);
    }

    construct
    {
      online = false;
    }

    /*
     * Public Methods
     */
    public void update_info (GLib.ValueArray value_array)
                             requires (value_array.n_values == 10)
    {
      /* Un-marshal the array and update our information */
      name = value_array.get_nth (1).get_string ();
      icon = value_array.get_nth (2).get_string ();
      position = value_array.get_nth (3).get_uint ();

      /* FIXME: Unmarshall string[] mimetypes */

      sensitive = value_array.get_nth (5).get_boolean ();
      sections_model_name = value_array.get_nth (6).get_string ();

      HashTable<string, string> hash = (HashTable<string, string>)(value_array.get_nth (7).get_boxed ());
      if (hints != null)
        hints = map_from_hash (hash);
      else
        hints = null;

      /* Unmarshal the Entry RenderInfo */
      unowned ValueArray ea = (ValueArray)(value_array.get_nth (8).get_boxed ());
      if (ea != null)
        {
          entry_renderer_name = ea.get_nth (0).get_string ();

          var str = ea.get_nth (1).get_string ();
          if (entry_groups_model_name != name)
            {
              entry_groups_model_name = name;
              entry_groups_model = null;
            }

          str = ea.get_nth (2).get_string ();
          if (entry_results_model_name != str)
            {
              entry_results_model_name = str;
              entry_results_model = null;
            }

          hash = (HashTable<string, string>)(ea.get_nth (3).get_boxed ());
          if (hash != null)
            entry_renderer_hints = map_from_hash (hash);
          else
            entry_renderer_hints = null;
        }

      /* Unmarshal the Global RenderInfo */
      unowned ValueArray ga = (ValueArray)(value_array.get_nth (9).get_boxed ());
      /* Because Mikkel hates me and sends null values for structs. I'll get
       * that Dane one day.
       */
      if (ga != null)
        {
          global_renderer_name = ga.get_nth (0).get_string ();

          var str = ga.get_nth (1).get_string ();
          if (global_groups_model_name != name)
            {
              global_groups_model_name = name;
              global_groups_model = null;
            }

          str = ga.get_nth (2).get_string ();
          if (global_results_model_name != str)
            {
              global_results_model_name = str;
              global_results_model = null;
            }

          hash = (HashTable<string, string>)(ga.get_nth (3).get_boxed ());
          if (hash != null)
            global_renderer_hints = map_from_hash (hash);
          else
            global_renderer_hints = null;
        }

      updated ();
      renderer_info_changed ();
    }

    public new void connect ()
    {
      if (online == true)
        return;

      /* Grab the entry off DBus */
      try {
        connection = DBus.Bus.get (DBus.BusType.SESSION);
        service = connection.get_object (dbus_name,
                                         dbus_path,
                                         "com.canonical.Unity.PlaceEntry");
      } catch (Error e) {
        warning (@"Unable to connect to $dbus_path on $dbus_name: %s",
                 e.message);
        return;
      }

      service.RendererInfoChanged.connect (on_renderer_info_changed);

      online = true;
    }

    public void set_search (string search, HashTable<string, string> hints)
    {
      uint id;

      id = service.set_search (search, hints);
    }

    public void set_active_section (uint section_id)
    {
      service.set_active_section (section_id);
    }

    public void set_global_search (string                    search,
                                   HashTable<string, string> hints)
    {
      service.set_global_search (search, hints);
    }

    /*
     * Private Methods
     */

    private void on_renderer_info_changed (dynamic DBus.Object dbus_object,
                                           RendererInfo        info)
    {
      RendererInfo *i = &info;
      unowned ValueArray ea = (ValueArray)i;

      if (ea != null)
        {
          entry_renderer_name = ea.get_nth (0).get_string ();

          var str = ea.get_nth (1).get_string ();
          if (entry_groups_model_name != name)
            {
              entry_groups_model_name = name;
              entry_groups_model = null;
            }

          str = ea.get_nth (2).get_string ();
          if (entry_results_model_name != str)
            {
              entry_results_model_name = str;
              entry_results_model = null;
            }

          HashTable<string, string>hash = (HashTable<string, string>)(ea.get_nth (3).get_boxed ());
          if (hash != null)
            entry_renderer_hints = map_from_hash (hash);
          else
            entry_renderer_hints = null;

          updated ();
          renderer_info_changed ();
        }
    }

    private Gee.HashMap<string, string> map_from_hash (HashTable<string, string>hash)
    {
      Gee.HashMap<string, string> map = new Gee.HashMap<string, string> ();

      HashTableIter<string, string> iter = HashTableIter<string, string> (hash);
      unowned void* key, val;
      while (iter.next (out key, out val))
        {
          unowned string k = (string)key;
          unowned string v = (string)val;

          map[k] = v;
          print (@"$k = $v");
        }

      return map;
    }
  }
}

