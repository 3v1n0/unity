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

namespace G
{
  namespace Test
  {
    public class Log
    {
      public delegate bool LogFatalFunc (string?       log_domain,
                                         LogLevelFlags flags,
                                         string?       message);

      public extern static void set_fatal_handler (LogFatalFunc func);
    }
  }
}

namespace Unity.Testing
{
  public class Logging
  {
    public Logging ()
    {
      Log.set_default_handler (log_handler);
    }

    public static bool fatal_handler (string?       log_domain,
                                      LogLevelFlags flags,
                                      string?       message)
    {
      if (log_domain == null)
        return true;

      if ("liblauncher" in log_domain)
        return false;

      if ("bamfdaemon" in log_domain)
        return false;

      if ("Gtk" in log_domain)
        return false;

      if ("ndicator" in log_domain)
        return false;


      if (message == null)
        return true;

      if ("widget class `GtkImage' has no property named `x-ayatana-indicator-dynamic'" in message)
        return false;

      if ("is currently inside an allocation cycle" in message)
        return false;

      if ("Bamf r" in message)
        return false;

      if ("Failed to fetch path" in message)
        return false;

      if ("WARNING" in log_domain)
        return false;

      return true;
    }

    /**
     * init_fatal_handler:
     * Makes sure that known warnings don't make our testing fail
     **/
    public static void init_fatal_handler ()
    {
      G.Test.Log.set_fatal_handler (fatal_handler);
    }

    private void log_handler (string? log_domain,
                              LogLevelFlags flags,
                              string        message)
    {
      string level;

      if (log_domain == "Clutter" && (flags & LogLevelFlags.LEVEL_WARNING)!=0)
        {
          if ("is currently inside an allocation cycle" in message)
            return;
        }

      if (log_domain == "Gtk" && (flags & LogLevelFlags.LEVEL_WARNING) != 0)
        {
          if ("has no property named `x-ayatana-indicator-dynamic'" in message)
            return;
        }

      if ((flags & LogLevelFlags.LEVEL_ERROR) != 0)
        level = "FATAL";
      else if ((flags & LogLevelFlags.LEVEL_CRITICAL) != 0)
        level = "CRITICAL";
      else if ((flags & LogLevelFlags.LEVEL_WARNING) != 0)
        level = "WARNING";
      else if ((flags & LogLevelFlags.LEVEL_MESSAGE) != 0)
        level = "MESSAGE";
      else if ((flags & LogLevelFlags.LEVEL_INFO) != 0)
        level = "INFO";
      else if ((flags & LogLevelFlags.LEVEL_DEBUG) != 0)
        level = "DEBUG";
      else
        level = @"$flags";

      print ("%s-%s: %s\n\n",
             log_domain == null ? "Unity" : log_domain,
             level,
             message);
    }
  }
}
