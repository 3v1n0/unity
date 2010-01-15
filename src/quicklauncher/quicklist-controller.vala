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
  
  public class QuicklistController : Object
  {
    public string label;
    public Ctk.Menu menu;
    Ctk.Actor attached_widget;
    
    public QuicklistController (string label, Ctk.Actor attached_to, 
                               Clutter.Stage stage)
    {
      this.label = label;
      this.attached_widget = attached_to;
      
      this.menu = new QuicklistMenu () as Ctk.Menu;
      Ctk.MenuItem menuitem = new Ctk.MenuItem.with_label (this.label);
      this.menu.append (menuitem);
      this.menu.attach_to_actor (this.attached_widget);
      
      stage.add_actor (this.menu);
      this.menu.show ();
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
      Ctk.MenuItem menuitem = new Ctk.MenuItem.with_label (shortcut.get_name ());
      if (is_secondary)
        {
          this.menu.prepend (menuitem);
        }
      else 
        {
          this.menu.append (menuitem);
        }
        
        menuitem.activated.connect (shortcut.activated);
        menuitem.activated.connect (this.close_menu);
    }
    
    private void close_menu ()
    {
      Clutter.Stage? stage = this.menu.get_stage() as Clutter.Stage;
      if (stage != null)
        {
          stage.remove_actor (this.menu);
        }
    }
    
  } 
}
