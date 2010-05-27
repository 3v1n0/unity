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
using Gee;
using Unity;
using Unity.Testing;
using Unity.Tests.UI;

namespace Unity.Tests.UI
{
public class TestFavorites : Unity.Favorites
  {
    public override ArrayList<string> get_favorites ()
    {
      var retlist = new ArrayList<string> ();
      retlist.add ("app-firefox");
      return retlist;
    }

    public override void add_favorite (string uid)
    {
    }
    public override void remove_favorite (string uid)
    {
    }
    public override bool is_favorite (string uid)
    {
      return true;
    }

    public override string? get_string (string uid, string name)
    {
      if (name == "type")
        return "application";

      return "/usr/share/applications/firefox.desktop";
    }

    public override void set_string (string uid, string name, string value)
    {
    }

    public override int? get_int (string uid, string name)
    {
      return null;
    }
    public override void set_int (string uid, string name, int value)
    {
    }


    public override float? get_float (string uid, string name)
    {
      return null;
    }
    public override void set_float (string uid, string name, float value)
    {
    }

    public override bool? get_bool (string uid, string name)
    {
      return null;
    }

    public override void set_bool (string uid, string name, bool value)
    {
    }
  }
}

public class Main
{
  public static int main (string[] args)
  {
    Logging        logger;
    QuicklistSuite quicklist_suite;
    //AutomationBasicTestSuite basic_test_suite;

    Environment.set_variable ("UNITY_DISABLE_TRAY", "1", true);
    Environment.set_variable ("UNITY_DISABLE_IDLES", "1", true);
    Environment.set_variable ("UNITY_PANEL_INDICATORS_SKIP", "all", true);

    Gtk.init (ref args);
    Gtk.Settings.get_default ().gtk_xft_dpi = 96 * 1024;

    Ctk.init (ref args);
    Test.init (ref args);

    logger = new Logging ();

    quicklist_suite = new QuicklistSuite ();
    //basic_test_suite = new AutomationBasicTestSuite ();

    Timeout.add_seconds (3, ()=> {
      Test.run ();
      Gtk.main_quit ();
      return false;
    });

    Gtk.main ();

    return 0;
  }
}
