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
  public QuicklistController? active_menu = null;

  public class QuicklistController : Object
  {
    public string label;
    public Ctk.Menu? menu;
    private Ctk.Menu? old_menu;
    private Clutter.Stage stage;
    private Ctk.Actor attached_widget;

    public bool is_label = false; // in label mode, don't add items
    private Gee.LinkedList<ShortcutItem> prefix_actions;
    private Gee.LinkedList<ShortcutItem> append_actions;

		private bool pointer_is_in_menu = false;
		private bool pointer_is_in_actor = false;

    public QuicklistController (string label, Ctk.Actor attached_to,
                               Clutter.Stage stage)
    {
      this.label = label;
      this.stage = stage;
      this.menu = null;
      this.prefix_actions = new Gee.LinkedList<ShortcutItem> ();
      this.append_actions = new Gee.LinkedList<ShortcutItem> ();
      this.attached_widget = attached_to;
      var drag_controller = Drag.Controller.get_default ();
      drag_controller.drag_start.connect (this.on_unity_drag_start);
    }

    ~QuicklistController ()
    {
      if (active_menu == this)
        {
          active_menu = null;
        }
      var menu = this.menu;
      this.menu = null;
      if (menu is Ctk.Menu)
        {
          menu.destroy ();
        }
    }

    construct
    {
    }

    private void on_unity_drag_start (Drag.Model model)
    {
      if (active_menu == this)
        {
          active_menu = null;
        }
      var menu = this.menu;
      this.menu = null;
      if (menu is Ctk.Menu)
        {
          menu.destroy ();
        }
    }

    public void add_action (ShortcutItem shortcut, bool is_secondary)
    {
      /* replace with Unity.Quicklauncher.MenuItem or something when we have
       * that ready, needs an activated () signal
       */
      if (is_secondary)
        this.prefix_actions.add (shortcut);
      else
        this.append_actions.add (shortcut);
    }

    private void build_menu ()
    {
      this.menu = new QuicklistMenu () as Ctk.Menu;
      this.menu.set_swallow_clicks (Unity.global_shell.menus_swallow_events);
      this.menu.set_detect_clicks (false);
      Unity.Quicklauncher.QuicklistMenuItem menuitem = new Unity.Quicklauncher.QuicklistMenuItem (this.label);
      menuitem.activated.connect (this.close_menu);
      this.menu.append (menuitem, true);
      this.menu.attach_to_actor (this.attached_widget);
      stage.add_actor (this.menu);

      float x;
      float y;
      this.menu.get_position (out x, out y);
      this.menu.set_position (x - (float) Ctk.em_to_pixel (1.5f), y);
    }

    public void show_label ()
    {
      // first of all check, if we have a menu active we don't show any labels
      if (Unity.Quicklauncher.active_menu != null)
        return;

      if (this.menu == null)
        this.build_menu ();

      this.menu.show();
      this.is_label = true;
      this.menu.set_detect_clicks (false);
    }

    public void hide_label ()
    {
      if (!is_label || menu == null)
        return;

      this.menu.set_detect_clicks (false);
      this.menu.hide ();
    }

    public void show_menu ()
    {
      if (Unity.Quicklauncher.active_menu != null)
        {
          // we already have an active menu, so destroy that and start this one
          Unity.Quicklauncher.active_menu.menu.destroy ();
          Unity.Quicklauncher.active_menu = null;
        }

      if (this.menu == null)
        {
          this.show_label ();
        }

      this.is_label = false;

      foreach (ShortcutItem shortcut in this.prefix_actions)
        {
          var label = shortcut.get_name ();
          Unity.Quicklauncher.QuicklistMenuItem menuitem = new Unity.Quicklauncher.QuicklistMenuItem (label);
          this.menu.prepend (menuitem, false);
          menuitem.activated.connect (shortcut.activated);
          menuitem.activated.connect (this.close_menu);
        }

      foreach (ShortcutItem shortcut in this.append_actions)
        {
          var label = shortcut.get_name ();
          Unity.Quicklauncher.QuicklistMenuItem menuitem = new Unity.Quicklauncher.QuicklistMenuItem (label);
          this.menu.append (menuitem, false);
          menuitem.activated.connect (shortcut.activated);
          menuitem.activated.connect (this.close_menu);
        }

      Unity.Quicklauncher.active_menu = this;
      this.menu.closed.connect (this.on_menu_close);
      this.is_label = false;
      this.menu.set_detect_clicks (true);

      Unity.global_shell.ensure_input_region ();
    }

    private void on_menu_close ()
    {
      close_menu ();
    }

    public void close_menu ()
    {
      if (Unity.Quicklauncher.active_menu == this)
        {
          Unity.Quicklauncher.active_menu = null;
        }

      if (this.menu == null)
        return;

      this.menu.destroy ();
      this.old_menu = this.menu;
      this.menu = null;
      this.is_label = false;

      Unity.global_shell.ensure_input_region ();
    }

  }
}
