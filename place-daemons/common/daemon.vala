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
 * Authored by Mikkel Kamstrup Erlandsen <mikkel.kamstrup@canonical.com>
 *
 */
using Dbus;
using DBus;

namespace Unity.Place {  

  [DBus (name = "com.canonical.Unity.Place")]
  public interface DBusPlace
  {
    public abstract void set_active (bool is_active) throws DBus.Error;

    public signal void view_changed (HashTable<string,string> properties);
  }

  public class Daemon : GLib.Object, DBusPlace
  {
    private Dbus.Model _results_model;
    private Dbus.Model _groups_model;
    public Dbus.Model results_model { get { return _results_model; } }
    public Dbus.Model groups_model { get { return _results_model; } }
    public string results_model_name { get; construct; }
    public string groups_model_name { get; construct; }
    public string place_name { get; construct; }
    public string place_path { get; construct; }
    
    public Daemon (string place_name, string place_path,
                   string results_model_name, string groups_model_name)
    {
      GLib.Object (place_name:place_name,
                   place_path:place_path,
                   results_model_name:results_model_name,
                   groups_model_name:groups_model_name);
      
      _results_model = new Dbus.Model (results_model_name, 5, typeof(string),
                                       typeof(string), typeof(string), 
                                       typeof(string), typeof(string));      
      
      _groups_model = new Dbus.Model (groups_model_name, 1, typeof(string));
      
      /* We should not do anything with the model
       * until we receieve the 'ready' signal */
      _results_model.ready.connect (this.on_results_model_ready);
      _results_model.connect ();
      
      // FIXME: _groups_model wait for 'ready'
      debug ("33333333");
    }    

    public virtual void on_results_model_ready (Dbus.Model model)
    {
      debug ("22222");
      emit_view_changed_signal();
    }

    public bool emit_view_changed_signal ()
    {
      var props = new HashTable<string, string> (str_hash, str_equal);

      props.insert ("view-name", "ResultsView");
      props.insert ("groups-model", groups_model_name);
      props.insert ("results-model", results_model_name);

      this.view_changed (props);
      return false;
    }

    /* Override: DBusPlace.set_active */
    public void set_active (bool is_active) throws DBus.Error
    {
      debug (@"Set active: $is_active");
      emit_view_changed_signal ();
    }
    
    public int run ()
    {
      /* Export the place daemon on the session bus */
      try {
        var conn = DBus.Bus.get (DBus.BusType.SESSION);
        dynamic DBus.Object bus = conn.get_object ("org.freedesktop.DBus",
                                                   "/org/freedesktop/DBus",
                                                   "org.freedesktop.DBus");

        uint request_name_result = bus.request_name (place_name, (uint) 0);
        if (request_name_result == DBus.RequestNameReply.PRIMARY_OWNER)
          {
            conn.register_object (place_path, this);
            debug ("Running place daemon '%s'", place_name);
            new MainLoop (null, false).run();
          }
        else
          {        
            print ("Another Place Daemon appears to be running on '%s'\nBailing out",
                   place_name);
            return 1;
          }
      } catch (DBus.Error error) {
        GLib.error ("Error connecting to session bus: %s\nBailing out", error.message);
        return 2;
      }
      
      return 0;
    }
  }
} /* namespace */