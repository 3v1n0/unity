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
 * Authored by Gordon Allott <gord.allott@canonical.com>
 *
 */
using Unity.Quicklauncher.Models;

namespace Unity.Quicklauncher
{
  public QuicklistController? ql_controler_singleton;

  public class QuicklistController : Object
  {

    public static unowned QuicklistController get_default ()
    {
      if (Unity.Quicklauncher.ql_controler_singleton == null) {
        Unity.Quicklauncher.ql_controler_singleton= new QuicklistController ();
      }
      return Unity.Quicklauncher.ql_controler_singleton;
    }

    public weak Ctk.Menu menu;
    public bool is_in_label = false;
    public bool is_in_menu = false;

    public QuicklistController ()
    {
    }

    construct
    {
      var drag_controller = Drag.Controller.get_default ();
      drag_controller.drag_start.connect (this.on_unity_drag_start);
    }

    private void on_unity_drag_start (Drag.Model model)
    {
      if (this.menu_is_open ())
        this.menu.destroy ();
    }

    public void show_label (string label, Ctk.Actor attached_widget)
    {
      if (this.menu_is_open ())
        this.menu.destroy ();

      var menu = new QuicklistMenu () as Ctk.Menu;
      this.menu = menu;
      this.menu.destroy.connect (() => {
        Unity.global_shell.remove_fullscreen_request (this);
      });
      this.menu.set_swallow_clicks (Unity.global_shell.menus_swallow_events);
      this.menu.set_detect_clicks (false);

      //make label
      var menuitem = new QuicklistMenuItem (label);
      menuitem.activated.connect (this.close_menu);
      this.menu.append (menuitem, true);
      this.menu.attach_to_actor (attached_widget);
      (attached_widget.get_stage () as Clutter.Stage).add_actor (this.menu);

      float x;
      float y;
      this.menu.get_position (out x, out y);
      this.menu.set_position (x - (float) Ctk.em_to_pixel (1.5f), y);
      this.is_in_label = true;
    }

    private Gee.ArrayList<ShortcutItem> prefix_cache;
    private Gee.ArrayList<ShortcutItem> affix_cache;

    public void show_menu (Gee.ArrayList<ShortcutItem> prefix_shortcuts,
                           Gee.ArrayList<ShortcutItem> affix_shortcuts,
                           bool hide_on_leave)
    {
      this.prefix_cache = prefix_shortcuts;
      this.affix_cache = affix_shortcuts;

      this.is_in_label = false;
      Unity.global_shell.add_fullscreen_request (this);
      this.menu.close_on_leave = hide_on_leave;
      foreach (ShortcutItem shortcut in prefix_shortcuts)
        {
          var label = shortcut.get_name ();
          var menuitem = new QuicklistMenuItem (label);
          this.menu.prepend (menuitem, false);
          menuitem.activated.connect (shortcut.activated);
          menuitem.activated.connect (this.close_menu);
        }

      foreach (ShortcutItem shortcut in affix_shortcuts)
        {
          var label = shortcut.get_name ();
          var menuitem = new QuicklistMenuItem (label);
          this.menu.append (menuitem, false);
          menuitem.activated.connect (shortcut.activated);
          menuitem.activated.connect (this.close_menu);
        }
      this.menu.set_detect_clicks (true);
    }

    public void close_menu ()
    {
      this.is_in_label = false;
      this.is_in_menu = false;
      if (this.menu is Ctk.Menu)
        {
          this.menu.destroy ();
        }
    }

    public bool menu_is_open ()
    {
      return this.menu is Ctk.Menu;
    }

    public Ctk.Actor get_attached_actor ()
    {
      return this.menu.get_attached_actor ();
    }
  }
}
