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

GLib.Timer gTimer;
static void
on_animation_started (Clutter.Animation? anim)
{
    //stdout.printf("Animation Started\n");
    gTimer.start ();
}

static void
on_animation_completed (Clutter.Animation? anim)
{
    //stdout.printf("Animation Completed\n");
    gTimer.stop ();
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
/*
      ObjectRegistry registry = ObjectRegistry.get_default ();

      Logging.init_fatal_handler ();

      QuicklistController qlcontroller = QuicklistController.get_default ();
      ScrollerModel scroller = registry.lookup ("UnityScrollerModel").get(0) as ScrollerModel;

      gTimer = new GLib.Timer();
      int DT = 2500;
      stdout.printf("\n");

      foreach (ScrollerChild launcher in scroller)
        {
          //launcher = scroller[i] as ScrollerChild;
          var dir = new Director (launcher.get_stage() as Clutter.Stage);

          qlcontroller.show_label ("Ubuntu Software Centre", launcher);

          Clutter.Animation? anim;
          launcher.opacity = 255;


          gTimer.start ();
          anim = launcher.animate (Clutter.AnimationMode.EASE_IN_SINE, 2500, "opacity", 0);
          //anim.started.connect (on_animation_started);
          anim.completed.connect (on_animation_completed);
          dir.do_wait_for_animation (launcher);

          float dt = (float)gTimer.elapsed ();
          float dt0 =  (float)DT/1000.0f;
          stdout.printf("Expected Duration: %2.3f, Observed Duration: %2.3f, Error: %2.3f%%\n", dt0, dt,
                        (dt - dt0)*100.0f/dt0);

          gTimer.start ();
          anim = launcher.animate (Clutter.AnimationMode.EASE_IN_SINE, 2500, "opacity", 255);
          //anim.started.connect (on_animation_started);
          anim.completed.connect (on_animation_completed);
          dir.do_wait_for_animation (launcher);

          dt = (float)gTimer.elapsed ();
          dt0 =  (float)DT/1000.0f;
          stdout.printf("Expected Duration: %2.3f, Observed Duration: %2.3f, Error: %2.3f%%\n", dt0, dt,
                        (dt - dt0)*100.0f/dt0);


          //dir.do_wait_for_timeout (5000);
        }

      //debug ("After Animation");
      qlcontroller.close_menu ();
*/
    }
  }
}
