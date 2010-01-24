/* -*- Mode: vala; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
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

namespace Unity.Panel.Indicators
{
  public class View : Ctk.Box
  {
    public View ()
    {
      Object (orientation:Ctk.Orientation.HORIZONTAL, spacing:6);
    }

    construct
    {
      Idle.add (this.load_indicators);
    }

    private bool load_indicators ()
    {
      print ("%s\n", INDICATORDIR);
      print ("%s\n", INDICATORICONSDIR);

      print ("Reading indicators from: %s\n", INDICATORDIR);

      var dir = File.new_for_path (INDICATORDIR);

      try
        {
          var e = dir.enumerate_children (FILE_ATTRIBUTE_STANDARD_NAME,
                                          0,
                                          null);

          FileInfo file_info;
          while ((file_info = e.next_file (null)) != null)
            {
              this.load_indicator (INDICATORDIR + file_info.get_name ());
            }
        }
      catch (Error error)
        {
          print ("Unable to read indicators: %s\n", error.message);
        }

      return false;
    }

    private void load_indicator (string filename)
    {
      Indicator.Object o;

      o = new Indicator.Object.from_file (filename);

      if (o is Indicator.Object)
        print ("Successful: %s\n", filename);
    }
  }
}
