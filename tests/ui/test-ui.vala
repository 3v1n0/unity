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

using Unity;
using Unity.Testing;

using Unity.Tests.UI;

public class Main
{
  public static int main (string[] args)
  {
    Logging        logger;
    QuicklistSuite quicklist_suite;

    Environment.set_variable ("UNITY_DISABLE_TRAY", "1", true);
    Environment.set_variable ("UNITY_DISABLE_IDLES", "1", true);

    Gtk.init (ref args);
    Gtk.Settings.get_default ().gtk_xft_dpi = 96 * 1024;

    Ctk.init (ref args);
    Test.init (ref args);

    logger = new Logging ();

    quicklist_suite = new QuicklistSuite ();

    Timeout.add_seconds (5, ()=> {
      Test.run ();
      //Gtk.main_quit ();
      return false;
    });

    Gtk.main ();

    return 0;
  }
}
