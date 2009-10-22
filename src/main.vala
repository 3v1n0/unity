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

public class Main
{
  public static int main (string[] args)
  {
    Unity.Window window;

    Gtk.init (ref args);
    GtkClutter.init (ref args);
    
    window = new Unity.Window ();
    window.set_fullscreen ();
    
    var quicklauncher = new Unity.QuickLauncher (window.stage);
    window.stage.show_all ();
    stdout.printf ("Hello World!\n");

    Gtk.main ();

    return 0;
  }
}
