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
    Unity.UnderlayWindow window;

    Gtk.init (ref args);
    GtkClutter.init (ref args);
    
    window = new Unity.UnderlayWindow (false, 0, 0);
        
    var quicklauncher = new Unity.Quicklauncher.Main (window.get_stage ());
    
    window.show ();
    Gtk.main ();

    return 0;
  }
}
