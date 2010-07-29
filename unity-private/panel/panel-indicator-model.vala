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
      if (_global_model == null)
      {
        _global_model = new IndicatorsFileModel ();
      }

      return _global_model;
    }

    /* This is only for testing mode, so we can inject a test model before the
     * program runs
     */
    public static void set_default (IndicatorsModel model)
    {
      _global_model = model;
    }

    /* The only method we really need */
    public abstract Gee.ArrayList<Indicator.Object> get_indicators ();
    public abstract string get_indicator_name (Indicator.Object o);
  }

  public class IndicatorsFileModel : IndicatorsModel
  {
    public static HashMap<string, int> indicator_order = null;
    public HashMap<Indicator.Object, string> indicator_map;
    public ArrayList<Indicator.Object> indicator_list;

    public IndicatorsFileModel ()
    {
      START_FUNCTION ();

      string skip_list;

      indicator_map = new Gee.HashMap<Indicator.Object, string> ();
      indicator_order = new Gee.HashMap<string, int> ();
      indicator_list = new Gee.ArrayList<Indicator.Object> ();

      /* Static order of indicators. We always want the session indicators
       * to be on the far right end of the panel. That is why it the session
       * indicator is the last one set in indicator_order.
       */
      indicator_order.set ("libappmenu.so", 1);
      indicator_order.set ("libapplication.so", 2);
      indicator_order.set ("libsoundmenu.so", 3);
      indicator_order.set ("libmessaging.so", 4);
      indicator_order.set ("libdatetime.so", 5);
      indicator_order.set ("libme.so", 6);
      indicator_order.set ("libsession.so", 7);

      /* Indicators we don't want to load */
      skip_list = Environment.get_variable ("UNITY_PANEL_INDICATORS_SKIP");
      if (skip_list == null)
        skip_list = "";

      if (skip_list == "all")
        {
          message ("Skipping all indicator loading");
          return;
        }

      /* We need to look for icons in an specific location */
      Gtk.IconTheme.get_default ().append_search_path (Config.INDICATORICONSDIR);

      /* Start loading 'em in. .so are located in  INDICATORDIR*/
      var dir = File.new_for_path (Config.INDICATORDIR);
      try
        {
          var e = dir.enumerate_children (FILE_ATTRIBUTE_STANDARD_NAME, 0,null);

          ArrayList<string> sos = new ArrayList<string> ();

          FileInfo file_info;
          while ((file_info = e.next_file (null)) != null)
            {
              string leaf = file_info.get_name ();

              if (leaf in skip_list)
                continue;

              if (leaf[leaf.len()-3:leaf.len()] == ".so")
                {
                  sos.add (leaf);
                }
            }

          /* Order the so's before we load them */
          sos.sort ((CompareFunc)indicator_sort_func);

          foreach (string leaf in sos)
            this.load_indicator (Config.INDICATORDIR + leaf, leaf);
        }
      catch (Error error)
        {
          print ("Unable to read indicators: %s\n", error.message);
        }

      END_FUNCTION ();
    }

    public static int indicator_sort_func (string a, string b)
    {
      return indicator_order[a] - indicator_order[b];
    }

    private void load_indicator (string filename, string leaf)
    {
      Indicator.Object o;

      o = new Indicator.Object.from_file (filename);

      if (o is Indicator.Object)
        {
          this.indicator_map[o] = leaf;
          indicator_list.add (o);
        }
      else
        {
          warning ("Unable to load %s\n", filename);
        }
    }

    public override ArrayList<Indicator.Object> get_indicators ()
    {
      return indicator_list;
    }

    public override string get_indicator_name (Indicator.Object o)
    {
      return indicator_map[o];
    }
  }
}

