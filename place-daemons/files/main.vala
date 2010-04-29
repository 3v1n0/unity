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

namespace Unity.Place {  
  
  public class FilesPlaceDaemon : Daemon
  {
    const string file_attribs = FILE_ATTRIBUTE_PREVIEW_ICON + "," +
                                FILE_ATTRIBUTE_STANDARD_ICON + "," +
                                FILE_ATTRIBUTE_THUMBNAIL_PATH;

    private Zeitgeist.Log log;

    public FilesPlaceDaemon()
    {
      base ("com.canonical.Unity.FilesPlace",
            "/com/canonical/unity/places/files",
            "com.canonical.Unity.FilesPlace.Results",
            "com.canonical.Unity.FilesPlace.Groups");
           

      // FIXME: We need to install some Zeitgeist.Monitors to track updates
      //        but that depends on https://code.launchpad.net/~mhr3/libzeitgeist/various-fixes/+merge/24338
      log = new Zeitgeist.Log();      
    }    

    public override void on_results_model_ready (Dbus.Model model)
    {
      debug ("111111111");
      emit_view_changed_signal();
      get_recent_files.begin();
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
        debug ("Got %u events", events.len);

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

              debug ("Got %s, %s, %s", su.get_uri(), su.get_mimetype(), icon);

              results_model.append (ResultColumns.NAME, su.get_text(),
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

    
    public static int main (string[] args)
    {
      Daemon daemon = new FilesPlaceDaemon ();
      return daemon.run();
    }
  }
} /* namespace */