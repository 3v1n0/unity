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
 * Authored by canonical.com
 *
 */

using Gee;
using Utils;

namespace Unity.Panel.Indicators
{
  public abstract class IndicatorsModel : Object
  {
    private static IndicatorsModel _global_model = null;

    public static IndicatorsModel get_default ()
    {
      stdout.printf ("--- IndicatorsModel: Calling singleton\n");
      if (_global_model == null)
      {
        stdout.printf ("--- IndicatorsModel: Create unique object\n");
        _global_model = new IndicatorsFileModel ();
      }

      stdout.printf ("--- IndicatorsModel: Return singleton\n");
      return _global_model;
    }

    /* This is only for testing mode, so we can inject a test model before the program runs */
    public static void set_default (IndicatorsModel model)
    {
      _global_model = model;
    }

    /* The only method we really need */
    public abstract Gee.Map<string, Indicator.Object> get_indicators ();
  }

  public class IndicatorsFileModel : IndicatorsModel
  {
    private HashMap<string, int> indicator_order;
    public Gee.Map <string, Indicator.Object> indicator_map;

    public IndicatorsFileModel ()
    {
      stdout.printf ("--- IndicatorsFileModel: Constructor\n");
      string skip_list;

      /* Static order of indicators. We always want the session indicators to be on the far right end of the
        panel. That is why it the session indicator is the last one set in indicator_order. */
      this.indicator_order.set ("libapplication.so", 1);
      this.indicator_order.set ("libsoundmenu.so", 2);
      this.indicator_order.set ("libmessaging.so", 3);
      this.indicator_order.set ("libdatetime.so", 4);
      this.indicator_order.set ("libme.so", 5);
      this.indicator_order.set ("libsession.so", 6);

      /* Indicators we don't want to load */
      skip_list = Environment.get_variable ("UNITY_PANEL_INDICATORS_SKIP");
      if (skip_list == null)
        skip_list = "";

      /* We need to look for icons in an specific location */
      Gtk.IconTheme.get_default ().append_search_path (INDICATORICONSDIR);

      /* Start loading 'em in. .so are located in  INDICATORDIR*/
      /* Create a directory that reference the location of the indicators .so files */
      var dir = File.new_for_path (INDICATORDIR);
      stdout.printf ("--- IndicatorsFileModel: try section\n");
      try
        {
          var e = dir.enumerate_children (FILE_ATTRIBUTE_STANDARD_NAME, 0, null);

          FileInfo file_info;
          while ((file_info = e.next_file (null)) != null)
            {
              string leaf = file_info.get_name ();

              stdout.printf ("--- Loading %s\n", leaf);

              /* do we need this check here? */
              if (leaf in skip_list || skip_list == "all")
                continue;

              /* Shouldn't we test for ".so" instead of just "so"? */
              if (leaf[leaf.len()-2:leaf.len()] == "so")
                {
                  /* Build the file name path to the .so file */
                  this.load_indicator (INDICATORDIR + file_info.get_name (), file_info.get_name ());
                }
            }
        }
      catch (Error error)
        {
          print ("Unable to read indicators: %s\n", error.message);
        }

      stdout.printf ("--- IndicatorsFileModel: End Constructor\n");
    }

    public override Gee.Map<string, Indicator.Object> get_indicators ()
    {
      return indicator_map;
    }

    private void load_indicator (string filename, string leaf)
    {
      Indicator.Object o;

      o = new Indicator.Object.from_file (filename);

      if (o is Indicator.Object)
        {
//           /* Create an indicator Item */
//           IndicatorItem i = new IndicatorItem ();
//           /* Set the indicator object into the indicator Item */
//           i.set_object (o);
//           /* Get the indicator position from indicator_order */
//           i.position = (int)this.indicator_order[leaf];
//           /* Add the indicator into the Map structure */
          this.indicator_map[leaf] = o;
        }
      else
        {
          warning ("Unable to load %s\n", filename);
        }
    }
  }
}

