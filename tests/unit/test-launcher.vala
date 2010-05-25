/*
 *      test-launcher.vala
 *      Copyright (C) 2010 Canonical Ltd
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 3 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 *
 *
 *      Authored by Gordon Allott <gord.allott@canonical.com>
 */
using Unity;
using Unity.Launcher;
using Unity.Testing;

/* TODO - add more tests once we have bamf and can test the controllers */
namespace Unity.Tests.Unit
{
	public class TestBamfApplication : Bamf.Application
	{
		public bool test_is_active = true;
		public bool test_is_urgent = false;
		public bool test_user_visible = true;
		public bool test_is_running = true;

		public string desktop_file = "firefox.desktop";
		public string name = "firefox";
		public string icon = "firefox";

		public TestBamfApplication ()
		{
			Object (path: "/null");
		}

		construct
		{
		}

		public new unowned string get_desktop_file ()
		{
			return desktop_file;
		}

		GLib.List<Object> temp_list;
		public new unowned GLib.List get_windows ()
		{
			temp_list = new GLib.List<Object> ();
			return temp_list;
		}

		public new GLib.Array get_xids ()
		{
			Array<uint32> retarray = new Array<uint32> (true, false, (uint)sizeof (uint32));
			return retarray;
		}

		public override GLib.List get_children ()
		{
			var temp_list_children = new GLib.List<Object> ();
			return temp_list_children;
		}

		public override string get_icon ()
		{
			return icon;
		}

		public override string get_name ()
		{
			return name;
		}

		public override bool is_active ()
		{
			return test_is_active;
		}

		public override bool is_running ()
		{
			return test_is_running;
		}

		public override bool is_urgent ()
		{
			return test_is_urgent;
		}

		public new bool user_visible ()
		{
			return test_user_visible;
		}

		public override string view_type ()
		{
			return "test";
		}
	}

  public class LauncherSuite
  {
    public LauncherSuite ()
    {
      Test.add_data_func ("/Unity/Launcher/TestScrollerModel", test_scroller_model);
			Test.add_data_func ("/Unity/Launcher/TestScrollerChildController", test_scroller_child_controller);
    }

    public class TestScrollerChild : ScrollerChild
    {
    }

    // very basic tests for scroller model, makes sure its list interface works
    // as expected
    private void test_scroller_model ()
    {
      ScrollerModel model = new ScrollerModel ();
      ScrollerChild child_a = new TestScrollerChild ();
      ScrollerChild child_b = new TestScrollerChild ();
      ScrollerChild child_c = new TestScrollerChild ();

      model.add (child_a);
      model.add (child_c);
      model.insert (child_b, 1);

			// make sure each model is in the correct position
      assert (model[1] == child_b);
      assert (model[2] == child_c);

			// make sure non of the children got lost somehow
      foreach (ScrollerChild child in model)
      {
        assert (child is ScrollerChild);
      }

      assert (child_a in model);

			// make sure that after removing a child, everything is still in the correct order
      model.remove (child_b);
      assert (model[1] == child_c);
    }

		private void test_scroller_child_controller ()
		{
			TestBamfApplication test_app = new TestBamfApplication ();
			ScrollerChild child = new TestScrollerChild ();
			ApplicationController controller = new ApplicationController (TESTDIR + "/data/test_desktop_file.desktop", child);

			test_app.name = "Test Application-New";
			test_app.desktop_file = TESTDIR + "/data/test_desktop_file.desktop";
			test_app.icon = TESTDIR + "/data/test_desktop_icon.png";
			test_app.test_is_active = true;
			test_app.test_is_urgent = false;
			test_app.test_user_visible = true;
			test_app.test_is_running = true;

			//assert that the controller set the model into the correct state
			assert (child.running == false);
			assert (child.active == false);
			assert (child.needs_attention == false);
			assert (controller.name == "Test Application");

			controller.attach_application (test_app);
			assert (child.running == true);
			assert (child.active == true);
			assert (controller.name == "Test Application-New");

			test_app.test_is_active = false;
			test_app.active_changed (test_app.test_is_active);
			assert (child.active == false);

			test_app.test_is_running = false;
			test_app.running_changed (test_app.test_is_running);
			assert (child.running == false);
			assert (controller.debug_is_application_attached ());
		}
  }
}
