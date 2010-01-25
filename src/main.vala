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
using Unity;

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
    null
  }
};

public class Main
{
  public static int main (string[] args)
  {
    Unity.Application    app;
    Unity.UnderlayWindow window;
    Unity.TimelineLogger.get_default(); // just inits the timer for logging
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
    // attempt to get a boot logging filename
    boot_logging_filename = Environment.get_variable ("UNITY_BOOTLOG_FILENAME");
    if (boot_logging_filename != null)
      {
        Unity.is_logging = true;
      }
    else
      {
        Unity.is_logging = false;
      }
    START_FUNCTION ();

    /* Parse options */
    LOGGER_START_PROCESS ("gtk_init");
    Gtk.init (ref args);
    LOGGER_END_PROCESS ("gtk_init");
    LOGGER_START_PROCESS ("ctk_init");
    Ctk.init (ref args);
    LOGGER_END_PROCESS ("ctk_init");

    /* Unique instancing */
    LOGGER_START_PROCESS ("unity_application_constructor");
    app = new Unity.Application ();
    LOGGER_END_PROCESS ("unity_application_constructor");

    string? disable_unique = Environment.get_variable ("UNITY_NO_UNIQUE");
    if (app.is_running && disable_unique == null)
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

    /* Things seem to be okay, load the main window */
    window = new Unity.UnderlayWindow (popup_mode, popup_width, popup_height);
    app.shell = window;
    LOGGER_START_PROCESS ("unity_underlay_window_show");
    window.show ();
    LOGGER_END_PROCESS ("unity_underlay_window_show");

    END_FUNCTION ();

    if (boot_logging_filename != null)
      {
        Timeout.add_seconds (1, () => {
          Unity.TimelineLogger.get_default().write_log (boot_logging_filename);
          return false;
        });
      }
    Gtk.main ();




    return 0;
  }
}
