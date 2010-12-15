/*
 * Copyright (C) 2009-2010 Canonical Ltd
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
 * Authored by Gord Allott <gord.allott@canonical.com>
 *
 */

namespace Perf
{
  public class ProcessInfo
  {
    public ProcessInfo (string name)
    {
      this.name = name;
      start = 0;
      end = 0;
    }
    public string name;
    public double start;
    public double end;
  }

  public static TimelineLogger? timeline_singleton;
  public static bool is_logging;

  public class TimelineLogger : Object
  {
    private Timer global_timer;
    private Gee.HashMap<string, ProcessInfo> process_map;

    public static unowned Perf.TimelineLogger get_default ()
    {
      if (Perf.timeline_singleton == null)
      {
        Perf.timeline_singleton = new Perf.TimelineLogger ();
      }

      return Perf.timeline_singleton;
    }

    construct
    {
      this.process_map = new Gee.HashMap<string, ProcessInfo> ();
      this.global_timer = new Timer ();
      this.global_timer.start ();
    }

    public void start_process (string name)
    {
      if (name in this.process_map.keys)
      {
        warning ("already started process: %s", name);
        return;
      }

      var info = new ProcessInfo (name);
      this.process_map[name] = info;
      info.start = this.global_timer.elapsed ();
    }

    public void end_process (string name)
    {
      double end_time = this.global_timer.elapsed ();
      print ("the end time is %f", end_time);

      if (name in this.process_map.keys)
      {
        this.process_map[name].end = end_time;
      }
      else
      {
        warning ("process %s not started", name);
      }
    }

    public void write_log (string filename)
    {
      debug ("Writing performance log file: %s...", filename);
      var log_file = File.new_for_path (filename);
      FileOutputStream file_stream;
      try
      {
        if (!log_file.query_exists (null)) {
          file_stream = log_file.create (FileCreateFlags.NONE, null);
        }
        else
        {
          file_stream = log_file.replace (null, false, FileCreateFlags.NONE, null);
        }

        var output_stream = new DataOutputStream (file_stream);

        foreach (ProcessInfo info in this.process_map.values)
        {
          string name = info.name.replace (",", ";");
          string outline = "%s, %f, %f\n".printf(name, info.start, info.end);
            output_stream.put_string (outline, null);
        }

        file_stream.close (null);
      } catch (Error e)
      {
        warning (e.message);
      }
      debug ("Done writing performance log file: %s", filename);
    }
  }
}
