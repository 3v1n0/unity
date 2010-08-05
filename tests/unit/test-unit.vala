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
    UnityPixbufCacheSuite unity_pixbuf_cache;

    LauncherSuite launcher;
    QuicklistSuite quicklists;

    PanelIndicatorObjectEntryViewSuite panel_object_entry_view_suite;
    PanelIndicatorObjectViewSuite panel_object_view_suite;

    PlacesPlaceFileModelSuite place_file_model;
    PlacesPlaceSuite places_place;
    PlacesSuite places;
    PlaceSuite place;
    PlaceBrowserSuite place_browser;
    IOSuite io;
    AppInfoManagerSuite appinfo_manager;

    Gtk.init (ref args);
    Ctk.init (ref args);
    Test.init (ref args);

    /* Libunity tests */
    unity_pixbuf_cache = new UnityPixbufCacheSuite ();

    /* Launcher tests */
    launcher = new LauncherSuite ();
    quicklists = new QuicklistSuite ();

    /* Panel tests */
    panel_object_entry_view_suite = new PanelIndicatorObjectEntryViewSuite ();
    panel_object_view_suite = new PanelIndicatorObjectViewSuite ();
    place_file_model = new PlacesPlaceFileModelSuite ();
    places_place = new PlacesPlaceSuite ();

    /* Places tests */
    places = new PlacesSuite ();
    place = new PlaceSuite ();
    place_browser = new PlaceBrowserSuite ();

    /* IO utility tests */
    io = new IOSuite ();
    appinfo_manager = new AppInfoManagerSuite ();

    Test.run ();

    return 0;
  }
}
