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
using Dbus;
using Zeitgeist;

namespace Unity.Foo {

  public class FilesPlaceDaemon
  {
    private Dbus.Model model;
    private Zeitgeist.Log log;

    public FilesPlaceDaemon()
    {
      model = new Dbus.Model ("com.canonical.Unity.FilesPlace", 5,
                              typeof(string), typeof(string), typeof(string),
                              typeof(string), typeof(string));
      model.connect ();

      /* Create connection to Zeitgeist, and asynchromously fetch the
       * recently use files */
      log = new Zeitgeist.Log();
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
        print ("Got %u events", events.len);

        append_events_sorted (events);
      } catch (Error e) {
        warning ("Error fetching recetnly used files: %s", e.message);
      }
    }

    /* Appends a set of Zeitgeist.Events to our Dbus.Model assuming that
     * these events are already sorted with descending timestamps */
    private void append_events_sorted (PtrArray events)
    {
      int i;

      for (i = 0; i < events.len; i++)
        {
          Zeitgeist.Event ev = (Zeitgeist.Event)events.index(i);
          if (ev.num_subjects() > 0)
            {              
              // FIXME: We only use the first subject...
              Zeitgeist.Subject su = ev.get_subject(0);
              message ("GOT %s", su.get_uri());
              model.append (ResultColumns.NAME, su.get_text(),
                            ResultColumns.COMMENT, su.get_uri(),
                            ResultColumns.ICON_NAME, get_icon_for_subject (su),
                            ResultColumns.GROUP, "Recently used files",
                            ResultColumns.URI, su.get_uri(),
                            -1);
            }
        }
    }

    private string get_icon_for_subject (Zeitgeist.Subject su)
    {
      File f = File.new_for_uri (su.get_uri());
      FileIcon icon = new FileIcon(f);

      return icon.to_string();
    }
    
    public static int main (string[] args)
    {
      new FilesPlaceDaemon();

      new MainLoop(null, false).run();
      
      return 0;
    }
  }
} /* namespace */