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

static void
on_animation_completed (Clutter.Animation? anim)
{
    stdout.printf("Animation Completed\n");
}

namespace Unity.Tests.UI
{
  public class AutomationBasicTestSuite : Object
  {
    private const string DOMAIN = "/UI/Quicklist";

    Unity.Testing.Window? window;
    Clutter.Stage?        stage;

    public AutomationBasicTestSuite ()
    {
      Logging.init_fatal_handler ();

      /* Testup the test window */
      window = new Unity.Testing.Window (true, 1024, 600);
      window.init_test_mode ();
      stage = window.stage;
      window.title = "Automation Tests";
      window.show_all ();

      Test.add_data_func (DOMAIN + "/Automation",
                          test_automation);


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

    private void test_automation ()
    {
      ObjectRegistry registry = ObjectRegistry.get_default ();

      Logging.init_fatal_handler ();

      QuicklistController qlcontroller = QuicklistController.get_default ();
      ScrollerModel scroller = registry.lookup ("UnityScrollerModel") as ScrollerModel;
      
      //ScrollerChild launcher;
      //int i = 0; 
      foreach (ScrollerChild launcher in scroller)
        {
          //launcher = scroller[i] as ScrollerChild;
          var dir = new Director (launcher.get_stage() as Clutter.Stage);

          qlcontroller.show_label ("Ubuntu Software Centre", launcher);

          Clutter.Animation? anim;
          launcher.opacity = 255;
          anim = launcher.animate (Clutter.AnimationMode.EASE_IN_SINE, 500, "opacity", 0);
          //anim.completed.connect (on_animation_completed);
          dir.do_wait_for_animation (launcher);
          anim = launcher.animate (Clutter.AnimationMode.EASE_IN_SINE, 500, "opacity", 255);
          dir.do_wait_for_animation (launcher);
          //dir.do_wait_for_timeout (5000);
        }
      
      //debug ("After Animation");               
      /* Clean up */
      qlcontroller.close_menu ();
    }
  }
}
