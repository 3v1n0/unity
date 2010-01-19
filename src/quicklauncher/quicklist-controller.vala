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
  public Ctk.Menu? active_menu = null;
  
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
    
    public QuicklistController (string label, Ctk.Actor attached_to, 
                               Clutter.Stage stage)
    {
      this.label = label;
      this.stage = stage;
      this.menu = null;
      this.prefix_actions = new Gee.LinkedList<ShortcutItem> ();
      this.append_actions = new Gee.LinkedList<ShortcutItem> ();
      this.attached_widget = attached_to;
    }
    
    ~QuicklistController () 
    {
      this.menu.fadeout_and_destroy ();
    }
    
    construct 
    {
    }
    
    public void add_action (ShortcutItem shortcut, bool is_secondary)
    {
      /* replace with Unity.Quicklauncher.MenuItem or something when we have 
       * that ready, needs an activated () signal
       */
      if (is_secondary)
        {
          this.prefix_actions.add (shortcut);
        }
      else 
        {
          this.append_actions.add (shortcut);
        }
    }
    
    private void build_menu ()
    {
      this.menu = new QuicklistMenu () as Ctk.Menu;
      Ctk.MenuItem menuitem = new Ctk.MenuItem.with_label (this.label);
      this.menu.prepend (menuitem);
      this.menu.attach_to_actor (this.attached_widget);
      stage.add_actor (this.menu);
    }
    
    public void show_label ()
    {
      // first of all check, if we have a menu active we don't show any labels
      if (Unity.Quicklauncher.active_menu != null)
        {
          return;
        }
      
      if (this.menu == null)
        {
          this.build_menu ();
        }
        
      this.menu.show();
      this.is_label = true;
    }
    
    public void show_menu ()
    {
      if (this.menu == null)
        {
          this.show_label ();
        }

      if (Unity.Quicklauncher.active_menu != null)
        {
          // we already have an active menu, so destroy that and start this one
          Unity.Quicklauncher.active_menu.fadeout_and_destroy ();
          Unity.Quicklauncher.active_menu = null;
        }
        
      this.is_label = false;
      foreach (ShortcutItem shortcut in this.prefix_actions)
        {
          var label = shortcut.get_name ();
          Ctk.MenuItem menuitem = new Ctk.MenuItem.with_label (label);
          this.menu.prepend (menuitem);
          menuitem.activated.connect (shortcut.activated);
          menuitem.activated.connect (this.close_menu);
        }
        
      foreach (ShortcutItem shortcut in this.append_actions)
        {
          var label = shortcut.get_name ();
          Ctk.MenuItem menuitem = new Ctk.MenuItem.with_label (label);
          this.menu.prepend (menuitem);
          menuitem.activated.connect (shortcut.activated);
          menuitem.activated.connect (this.close_menu);
        }
        
      Unity.Quicklauncher.active_menu = this.menu;
      this.menu.set_detect_clicks (true);
      this.menu.closed.connect (this.on_menu_close);
      this.is_label = false;
    }
    
    private void on_menu_close ()
    {
      if (Unity.Quicklauncher.active_menu == this.menu)
        {
          Unity.Quicklauncher.active_menu = null;
        }
      this.is_label = false;
      this.old_menu = this.menu;
      this.menu = null;
    }
    
    public void close_menu ()
    {
      this.menu.fadeout_and_destroy ();
      this.old_menu = this.menu;
      this.menu = null;
      this.is_label = false;
    }
    
  } 
}
