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
using Zeitgeist;

namespace Unity.Foo {  

  /* Where to export the Unity controller */
  private const string dbus_place_name = "com.canonical.Unity.FilesPlace";
  private const string dbus_place_path = "/com/canonical/unity/places/files";

  [DBus (name = "com.canonical.Unity.Place")]
  public interface DBusPlace
  {
    public abstract void set_active (bool is_active) throws DBus.Error;

    public signal void view_changed (HashTable<string,string> properties);
  }
  
  public class FilesPlaceDaemon : GLib.Object, DBusPlace
  {
    private Dbus.Model model;
    private Zeitgeist.Log log;

    const string file_attribs = FILE_ATTRIBUTE_PREVIEW_ICON + "," +
                                  FILE_ATTRIBUTE_STANDARD_ICON + "," +
                                  FILE_ATTRIBUTE_THUMBNAIL_PATH;

    /* Names for the DbusModels we export */
    private const string results_model_name = "com.canonical.Unity.FilesPlace.Results";
    private const string groups_model_name  = "com.canonical.Unity.FilesPlace.Groups";

    public FilesPlaceDaemon()
    {
      model = new Dbus.Model (results_model_name, 5,
                              typeof(string), typeof(string), typeof(string),
                              typeof(string), typeof(string));      

      log = new Zeitgeist.Log();

      /* We should not do anything with the model
       * until we receieve the 'ready' signal */
      model.ready.connect (this.on_results_model_ready);
      model.connect ();      
    }    

    private void on_results_model_ready (Dbus.Model model)
    {
      emit_view_changed_signal();
      get_recent_files.begin();
    }

    private bool emit_view_changed_signal ()
    {
      var props = new HashTable<string, string> (str_hash, str_equal);

      props.insert ("view-name", "ResultsView");
      props.insert ("groups-model", groups_model_name);
      props.insert ("results-model", results_model_name);

      this.view_changed (props);
      return false;
    }
    
    private async void get_recent_files ()
    {
      try {
      
        PtrArray templates = new PtrArray();
        templates.add (new Zeitgeist.Event ());
        
        PtrArray events = yield log.find_events (
                                        new Zeitgeist.TimeRange.anytime(),
                                        (owned)templates,
                                        Zeitgeist.StorageState.ANY,
                                        20,
                                        Zeitgeist.ResultType.MOST_RECENT_SUBJECTS,
                                        null);
        print ("Got %u events", events.len);

        yield append_events_sorted ((owned)events);
      } catch (GLib.Error e) {
        warning ("Error fetching recetnly used files: %s", e.message);
      }
    }

    /* Appends a set of Zeitgeist.Events to our Dbus.Model assuming that
     * these events are already sorted with descending timestamps */
    private async void append_events_sorted (owned PtrArray events)
    {
      int i;

      for (i = 0; i < events.len; i++)
        {
          Zeitgeist.Event ev = (Zeitgeist.Event)events.index(i);
          if (ev.num_subjects() > 0)
            {              
              // FIXME: We only use the first subject...
              Zeitgeist.Subject su = ev.get_subject(0);              

              string icon = yield get_icon_for_subject (su);

              message ("GOT %s, %s, %s", su.get_uri(), su.get_mimetype(), icon);
              
              model.append (ResultColumns.NAME, su.get_text(),
                            ResultColumns.COMMENT, su.get_uri(),
                            ResultColumns.ICON_NAME, icon,
                            ResultColumns.GROUP, "Recently used files",
                            ResultColumns.URI, su.get_uri(),
                            -1);
            }
        }
    }

    /* Async method to query GIO for a good icon for a Zeitgeist.Subject */
    private async string get_icon_for_subject (Zeitgeist.Subject su)
    {

      // FIXME: It's a bit broken by design that we guess a file icon in the controller/model
      
      try {
      
        File f = File.new_for_uri (su.get_uri());
        FileInfo info = yield f.query_info_async (file_attribs,
                                                  FileQueryInfoFlags.NONE,
                                                  Priority.LOW,
                                                  null);
        Icon icon = info.get_icon();
        string thumbnail_path = info.get_attribute_byte_string (FILE_ATTRIBUTE_THUMBNAIL_PATH);

        if (thumbnail_path == null)
          return icon.to_string();
        else
          return thumbnail_path;
      } catch (GLib.Error e) {
        /* We failed to probe the icon. Try looking up by mimetype instead */
        Icon icon2 = g_content_type_get_icon (su.get_mimetype ());
        return icon2.to_string ();
      }
    }

    /* Override: DBusPlace.set_active */
    public void set_active (bool is_active) throws DBus.Error
    {
      debug (@"Set active: $is_active");
      emit_view_changed_signal ();
    }
    
    public static int main (string[] args)
    {
      /* Export the Unity controller on the session bus */
      try {
        var conn = DBus.Bus.get (DBus.BusType.SESSION);
        dynamic DBus.Object bus = conn.get_object ("org.freedesktop.DBus",
                                                   "/org/freedesktop/DBus",
                                                   "org.freedesktop.DBus");

        uint request_name_result = bus.request_name (dbus_place_name, (uint) 0);
        if (request_name_result == DBus.RequestNameReply.PRIMARY_OWNER)
          {
            var daemon = new FilesPlaceDaemon();
            conn.register_object (dbus_place_path, daemon);
            new MainLoop (null, false).run();
          }
        else
          {        
            print ("Another Unity Files Daemon appears to be running\nBailing out");
            return 1;
          }
      } catch (DBus.Error error) {
        GLib.error ("Error connecting to session bus: %s\nBailing out", error.message);
      }

      
      
      return 0;
    }
  }
} /* namespace */