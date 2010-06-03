/* -*- Mode: vala; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
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
 * Authored by canonical.com
 *
 */

public class MenuManager : Object
{
  private Gtk.Menu current_menu;
  private static MenuManager _menu_manager_global = null;

  public static MenuManager get_default ()
  {
    if (_menu_manager_global == null)
      _menu_manager_global = new MenuManager ();

    return _menu_manager_global;
  }

  public void register_visible_menu (Gtk.Menu menu)
  {
    if (current_menu is Gtk.Menu && (current_menu.visible == true) && (current_menu != menu))
      current_menu.popdown ();

    current_menu = menu;
  }

  
  public bool menu_is_open ()
  {
    if (current_menu.visible == true)
      {
        return true;
      }
    return false;
  }
}

