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
using Unity.Launcher;

namespace Unity.Tests.UI
{
  public class QuicklistSuite : Object
  {
    private const string DOMAIN = "/UI/Quicklist";

    Unity.Testing.Window? window;
    Clutter.Stage?        stage;

    public QuicklistSuite ()
    {
      Logging.init_fatal_handler ();

      /* Testup the test window */
      Unity.favorites_singleton = new TestFavorites ();
      window = new Unity.Testing.Window (true, 1024, 600);
      window.init_test_mode ();
      stage = window.stage;
      window.title = "Quicklist Tests";
      window.show_all ();

      Test.add_data_func (DOMAIN + "/ControllerShowLabel",
                          test_controller_show_label);


      /* Keep this one last, it's a dummy to clean up the state as Vala cant
       * deal with the standard TestSuite stuff properly
       */
      Test.add_data_func (DOMAIN +"/Teardown", test_teardown);
    }

    private void test_teardown ()
    {
      //window.destroy ();
      stage = null;
    }

    private void test_controller_show_label ()
    {
/*
      string img = TESTDIR + "/data/quicklist_controller_show_label.png";
      ObjectRegistry registry = ObjectRegistry.get_default ();

      Logging.init_fatal_handler ();

      ScrollerModel scroller = (registry.lookup ("UnityScrollerModel")[0]) as ScrollerModel;
      ScrollerChild first = scroller[0] as ScrollerChild;

      QuicklistController qlcontroller = QuicklistController.get_default ();
      qlcontroller.show_label ("Ubuntu Software Centre", first);

      while (Gtk.events_pending ())
        Gtk.main_iteration ();

      assert (Utils.compare_snapshot (stage, img, 54, 30, 200, 50));

      qlcontroller.close_menu ();
    */
    }

  }
}
