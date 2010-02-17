/* -*- Mode: vala; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/*
 * Copyright (C) 2009 Canonical Ltd
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

public class Main
{
  public const string firefox_desktop = Unity.Tests.TESTDIR+"/firefox.desktop";
  public static int main (string[] args)
  {
    Gtk.init (ref args);
    Ctk.init (ref args);
    Test.init (ref args);

    add_launcher_tests ();

    Idle.add (() => {
        Test.run ();
        Gtk.main_quit ();
        return false;
        }
      );

    Gtk.main ();

    return 0;
  }

  private static void add_launcher_tests ()
  {
    Test.add_func ("/Unity/Window", () => {
      var window = new Unity.UnderlayWindow (false, 0, 0);

      assert (window is Gtk.Window);

      window.show ();
      assert (window.visible);
    });


    Test.add_func ("/Unity/Background", () => {
      var bg = new Unity.Background ();
      assert (bg is Clutter.Actor);
    });
    Test.add_func ("/Unity/Quicklauncher/ApplicationModel", () => {
      var model = new Quicklauncher.Models.ApplicationModel (firefox_desktop);
      assert (model is Quicklauncher.Models.ApplicationModel);
    });

    Test.add_func ("/Unity/Quicklauncher/LauncherView", () => {
      var model = new Quicklauncher.Models.ApplicationModel (firefox_desktop);
      var view = new Quicklauncher.LauncherView (model);
      assert (view is Quicklauncher.LauncherView);
    });

    Test.add_func ("/Unity/Quicklauncher/Quicklist-view", () => {
      var view = new Quicklauncher.QuicklistMenu ();
      assert (view is Quicklauncher.QuicklistMenu);
    });

    Test.add_func ("/Unity/Quicklauncher/Manager", () => {
      var manager = new Quicklauncher.Manager ();
      assert (manager is Quicklauncher.Manager);
    });

    Test.add_func ("/Unity/Quicklauncher/Prism", () => {
      var webapp = new Quicklauncher.Prism ("http://www.google.com", "/tmp/icon.svg");
      assert (webapp is Quicklauncher.Prism);
      });

    Test.add_func ("/Unity/Widgets/Scroller", () => {
      var scroller = new Widgets.Scroller (Ctk.Orientation.VERTICAL, 0);
      assert (scroller is Widgets.Scroller);

      scroller = new Widgets.Scroller (Ctk.Orientation.HORIZONTAL, 0);
      assert (scroller is Widgets.Scroller);
    });

    Test.add_func ("/Unity/Places/TestPlace", () => {
      var place = new TestPlace ();

      assert (place is TestPlace);

      var loop = new MainLoop (null, false);

      try
        {
          DBus.Connection conn = DBus.Bus.get (DBus.BusType.SESSION);

          Utils.register_object_on_dbus (conn,
                                  "/com/canonical/Unity/Place",
                                  place);

          loop.run ();
        }
      catch (Error e)
        {
          warning ("TestPlace error: %s", e.message);
        }
    });
  }

  public class TestPlace : Unity.Place
  {
    public TestPlace ()
    {
      Object (name:"neil", icon_name:"gtk-apply", tooltip:"hello");
    }

    construct
    {
      this.is_active.connect ((a) => {print (@"$a\n");});
    }
  }
}
