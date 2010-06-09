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
    LauncherSuite launcher;

    PanelIndicatorObjectEntryViewSuite panel_object_entry_view_suite;
    PanelIndicatorObjectViewSuite panel_object_view_suite;

    PlacesPlaceFileModel place_file_model;
    PlacesSuite places;
    PlaceSuite place;

    Gtk.init (ref args);
    Ctk.init (ref args);
    Test.init (ref args);

    /* Launcher tests */
    launcher = new LauncherSuite ();

    /* Panel tests */
    panel_object_entry_view_suite = new PanelIndicatorObjectEntryViewSuite ();
    panel_object_view_suite = new PanelIndicatorObjectViewSuite ();
    place_file_model = new PlacesPlaceFileModel ();

    /* Places tests */
    places = new PlacesSuite ();
    place = new PlaceSuite ();

    Test.run ();

    return 0;
  }
}
