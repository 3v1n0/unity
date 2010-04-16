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
using Unity.Quicklauncher;
using Unity.Widgets;

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
      window = new Unity.Testing.Window (true, 1024, 600);
      window.init_test_mode ();
      stage = window.stage;
      window.title = "Quicklist Tests";
      window.show_all ();

      Test.add_data_func (DOMAIN + "/ControllerShowLabel",
                          test_controller_show_label);

      Test.add_data_func (DOMAIN + "/ShownOnHover",
                          test_shown_on_hover);

      /* Keep this one last, it's a dummy to clean up the state as Vala cant
       * deal with the standard TestSuite stuff properly
       */
      Test.add_data_func (DOMAIN +"/Teardown", test_teardown);
    }

    private void test_teardown ()
    {
      window.destroy ();
      window = null;
      stage = null;
    }

    private void test_controller_show_label ()
    {
      string img = TESTDIR + "/data/quicklist_controller_show_label.png";
      ObjectRegistry registry = ObjectRegistry.get_default ();

      Logging.init_fatal_handler ();

      Scroller scroller = registry.lookup ("UnityWidgetsScroller") as Scroller;
      ScrollerChild first = scroller.nth (0) as ScrollerChild;

      QuicklistController qlcontroller = QuicklistController.get_default ();
      qlcontroller.show_label ("Ubuntu Software Centre",
                               first.child as Ctk.Actor);

      assert (Utils.compare_snapshot (stage, img, 54, 30, 200, 50));

      /* Clean up */
      qlcontroller.close_menu ();
    }

    private void test_shown_on_hover ()
    {
      string img = TESTDIR + "/data/quicklist_shown_on_hover.png";
      ObjectRegistry registry = ObjectRegistry.get_default ();
      Director director = new Director (stage);

      Logging.init_fatal_handler ();

      /* Used when setting up the test */
      //Utils.save_snapshot (stage, img, 54, 30, 200, 50);

      Scroller scroller = registry.lookup ("UnityWidgetsScroller") as Scroller;
      Clutter.Actor first = scroller.nth (0).child;

      /* So, in this test we're not sure what the label is of the first scroller
       * -child so our control img is just a blank space. Instead of testing
       * that two images are similar, we're testing that the two images are
       * different, so we can be confident something happened on hover
       *
       * The added 'false' to compare_snapshot tells that function that the
       * expected result is that the test will fail (so it adjusts return
       * values to avoid extra code our end)
       */
      director.enter_event (first, 5, 5);
      assert (Utils.compare_snapshot (stage, img, 54, 30, 200, 50, false));

      /* Clean up */
      director.leave_event (first, 5, 5);
    }
  }
}
