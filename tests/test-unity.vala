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

    Test.add_func ("/Unity/Quicklauncher/ApplicationView", () => {
      var app = new Launcher.Application.from_desktop_file (
                    "/usr/share/applications/firefox.desktop");

  		var appview = new Quicklauncher.ApplicationView (app);
      assert (appview is Quicklauncher.ApplicationView);
      assert (appview.app is Launcher.Application);
		});

    Test.add_func ("/Unity/Quicklauncher/ApplicationStore", () => {
      var appstore = new Quicklauncher.ApplicationStore ();
      assert (appstore is Quicklauncher.ApplicationStore);
    });

    Test.add_func ("/Unity/Widgets/Scroller", () => {
      var scroller = new Widgets.Scroller (Ctk.Orientation.VERTICAL, 0);
      assert (scroller is Widgets.Scroller);

      scroller = new Widgets.Scroller (Ctk.Orientation.HORIZONTAL, 0);
      assert (scroller is Widgets.Scroller);
    });
  }
}
