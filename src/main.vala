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

static bool show_unity   = false;
static bool popup_mode   = false;
static int  popup_width  = 1024;
static int  popup_height = 600;
static bool show_version = false;
static string? boot_logging_filename = null;

const OptionEntry[] options = {
  {
    "show",
    's',
    0,
    OptionArg.NONE,
    ref show_unity,
    "Show Unity to the user",
    null
  },
  {
    "popup",
    'p',
    0,
    OptionArg.NONE,
    ref popup_mode,
    "Popup the Unity window (for debugging)",
    null
  },
  {
    "width",
    'w',
    0,
    OptionArg.INT,
    ref popup_width,
    "Width of Unity window (use with --popup/-p). Default: 1024",
    null
  },
  {
    "height",
    'h',
    0,
    OptionArg.INT,
    ref popup_height,
    "Height of Unity window (use with --popup/-p). Default: 600",
    null
  },
  {
    "version",
    'v',
    0,
    OptionArg.NONE,
    ref show_version,
    "Show the version and exit",
    null
  },
  {
    "enable-boot-logging",
    'b',
    OptionFlags.FILENAME,
    OptionArg.FILENAME,
    ref boot_logging_filename,
    "show the boobady bah and foo",
    null
  },
  {
    null
  }
};

public class Main
{
  public static int main (string[] args)
  {
    Unity.Application    app;
    Unity.UnderlayWindow window;

    /* Parse options */
    Gtk.init (ref args);
    Ctk.init (ref args);

    try
      {
        var opt_context = new OptionContext ("-- Unity");
        opt_context.set_help_enabled (true);
        opt_context.add_main_entries (options, null);
        opt_context.parse (ref args);
      }
    catch (OptionError e)
      {
        warning ("Unable to parse arguments: %s", e.message);
        warning ("Run '%s --help' to see a full list of available command line options",
                 args[0]);
        return 1;
      }

    if (show_version)
      {
        /* FIXME: Add VERSION define */
        print ("\nUnity %s\n", "0.1.0");
        return 0;
      }

    /* Unique instancing */
    Unity.TimelineLogger.get_default().start_process ("/Unity");
    app = new Unity.Application ();
    if (app.is_running)
      {
        Unique.Response response = Unique.Response.OK;
        
        if (show_unity)
          {
            print ("Showing Unity window\n");
            response = app.send_message (Unity.ApplicationCommands.SHOW, null);
          }
        else
          {
            print ("There already another instance of Unity running\n");
          }
        
        return response == Unique.Response.OK ? 0 : 1;
      }
    Unity.TimelineLogger.get_default().end_process ("/Unity");

    /* Things seem to be okay, load the main window */
    Unity.TimelineLogger.get_default().start_process ("/Unity/Window");
    window = new Unity.UnderlayWindow (popup_mode, popup_width, popup_height);
    app.shell = window;
    window.show ();
    Unity.TimelineLogger.get_default().end_process ("/Unity/Window");
    
    if (boot_logging_filename != null)
    {
      Timeout.add_seconds (3, () => {
        Unity.TimelineLogger.get_default().write_log ("stats.log");
        return false;
      });
    }
    Gtk.main ();

    
    

    return 0;
  }
}
