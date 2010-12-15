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

using Unity.Testing;
using Unity.Tests.Unit;

public class Main
{
  public static int main (string[] args)
  {
    PlaceSuite place;
    PlaceBrowserSuite place_browser;
    IOSuite io;
    AppInfoManagerSuite appinfo_manager;

    Gtk.init (ref args);
    Test.init (ref args);

    /* Places tests */
    place = new PlaceSuite ();
    place_browser = new PlaceBrowserSuite ();

    /* IO utility tests */
    io = new IOSuite ();
    appinfo_manager = new AppInfoManagerSuite ();

    Test.run ();

    return 0;
  }
}
