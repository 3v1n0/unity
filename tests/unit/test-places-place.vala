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
using Unity.Places;
using GLib.Test;

namespace Unity.Tests.Unit
{
  public class PlacesPlaceSuite : Object
  {
    public const string DOMAIN = "/Unit/Places/Place";

    public PlacesPlaceSuite ()
    {
      Logging.init_fatal_handler ();

      Test.add_data_func (DOMAIN + "/Allocation", test_allocation);
      Test.add_data_func (DOMAIN + "/SimplePlaceFile", test_simple_placefile);
      Test.add_data_func (DOMAIN + "/AdvancedPlaceFile", test_advanced_place_file);
      Test.add_data_func (DOMAIN + "/BadSimplePlaceFile", test_bad_simple_place_file);
      Test.add_data_func (DOMAIN + "/BadAdvancedPlaceFile", test_bad_advanced_place_file);
    }

    private void test_allocation ()
    {
      var place = new Places.Place ("__name__", "__path__");
      assert (place is Places.Place);
      assert (place.dbus_name == "__name__");
      assert (place.dbus_path == "__path__");
    }

    private void test_simple_placefile ()
    {
      var file = new KeyFile ();
      try {
        file.load_from_file (TESTDIR + "/data/place0.place",
                             KeyFileFlags.NONE);
      } catch (Error e) {
          error ("Unable to load test place: %s", e.message);
      }

      var place = Places.Place.new_from_keyfile (file);
      assert (place is Places.Place);

      /* Test Place group was loaded properly */
      assert (place.dbus_name == "org.ayatana.Unity.Place0");
      assert (place.dbus_path == "/org/ayatana/Unity/Place0");

      /* Test entries were loaded properly */
      assert (place.n_entries == 0);
    }

    private void test_advanced_place_file ()
    {
      var file = new KeyFile ();
      try {
        file.load_from_file (TESTDIR + "/data/place1.place",
                             KeyFileFlags.NONE);
      } catch (Error error) {
          warning ("Unable to load test place: %s", error.message);
      }

      var place = Places.Place.new_from_keyfile (file);
      assert (place is Places.Place);

      /* Test Place group was loaded properly */
      assert (place.dbus_name == "org.ayatana.Unity.Place1");
      assert (place.dbus_path == "/org/ayatana/Unity/Place1");

      /* Test entries were loaded properly */
      assert (place.n_entries == 3);

      /* Test individual entry's properties were loaded correctly */
      PlaceEntry? e;

      e = place.get_nth_entry (0);
      assert (e is PlaceEntry);
      assert (e.dbus_path == "/org/ayatana/Unity/Place1/Entry1");
      assert (e.name == "One");
      assert (e.icon == "gtk-apply");
      assert (e.description == "One Description");
      assert (e.show_global == true);
      assert (e.show_entry == false);

      e = place.get_nth_entry (1);
      assert (e is PlaceEntry);
      assert (e.dbus_path == "/org/ayatana/Unity/Place1/Entry2");
      assert (e.name == "Two");
      assert (e.icon == "gtk-close");
      assert (e.description == "Two Description");
      assert (e.show_global == false);
      assert (e.show_entry == true);

      e = place.get_nth_entry (2);
      assert (e is PlaceEntry);
      assert (e.dbus_path == "/org/ayatana/Unity/Place1/Entry3");
      assert (e.name == "Three");
      assert (e.icon == "gtk-cancel");
      assert (e.description == "Three Description");
      assert (e.show_global == false);
      assert (e.show_entry == false);
    }

    private void test_bad_simple_place_file ()
    {
      var file = new KeyFile ();
      try {
        file.load_from_file (TESTDIR + "/data/place0.badplace",
                             KeyFileFlags.NONE);
      } catch (Error error) {
          warning ("Unable to load test place: %s", error.message);
      }

      if (trap_fork (0, TestTrapFlags.SILENCE_STDOUT | TestTrapFlags.SILENCE_STDERR))
        {
          var place = Places.Place.new_from_keyfile (file);
          assert (place is Places.Place);
          Posix.exit (0);
        }
        trap_has_passed ();
        trap_assert_stderr ("*Does not contain 'Place' grou*");
    }

    private void test_bad_advanced_place_file ()
    {
      var file = new KeyFile ();
      try {
        file.load_from_file (TESTDIR + "/data/place1.badplace",
                             KeyFileFlags.NONE);
      } catch (Error error) {
          warning ("Unable to load test place: %s", error.message);
      }

      if (trap_fork (0, TestTrapFlags.SILENCE_STDOUT | TestTrapFlags.SILENCE_STDERR))
        {
          var place = Places.Place.new_from_keyfile (file);
          assert (place is Places.Place);
          Posix.exit (0);
        }
        trap_has_passed ();
        trap_assert_stderr ("*Does not contain valid DBusObjectPat*");
    }
  }
}
