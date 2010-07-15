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
namespace Unity.Launcher
{
  public static QuicklistController? ql_controller_singleton;

  public enum QuicklistControllerState {
    LABEL,
    MENU,
    CLOSED
  }

  public abstract class QuicklistController : Object
  {
    protected Ctk.Menu? menu;
    public ScrollerChildController attached_controller {get; construct;}
    private QuicklistControllerState _state;
    public QuicklistControllerState state {
      get { return _state; }
      set
        {
          if (value == QuicklistControllerState.LABEL ||
              value == QuicklistControllerState.MENU)
            {
              // check our singleton for a previous menu
              if (ql_controller_singleton != this && ql_controller_singleton != null)
                {
                  var tmp_qlc = ql_controller_singleton; //ref the controller
                  tmp_qlc.state = QuicklistControllerState.CLOSED;
                  if (tmp_qlc.get_view () != null)
                    warning ("old menu not destroyed when opening new menu");
                }


              ql_controller_singleton = this;
            }

          if (value == QuicklistControllerState.CLOSED)
            {
              if (menu is Ctk.Menu)
                {
                  menu.destroy ();
                  menu = null;
                }
              ql_controller_singleton = null;
            }

          _state = value;
        }
    }

    public static unowned QuicklistController get_current_menu ()
    {
      // returns the current controller, thou does not own this!
      return ql_controller_singleton;
    }


    public static bool is_menu_open ()
    {
      // returns true if a menu is open
      if (ql_controller_singleton is QuicklistController)
        return ql_controller_singleton.state == QuicklistControllerState.MENU;

      return false;
    }

    public static bool do_menus_match (QuicklistController menu)
    {
      // returns true if the given menu matches ql_controller_singleton
      return menu == ql_controller_singleton;
    }

    public unowned Ctk.Menu? get_view ()
    {
      return menu;
    }

  }


  public class ApplicationQuicklistController : QuicklistController
  {
    public ApplicationQuicklistController (ScrollerChildController scroller_child)
    {
      Object (attached_controller: scroller_child);
    }

    construct
    {
      Unity.Testing.ObjectRegistry.get_default ().register ("QuicklistController",
                                                            this);

      new_menu ();
      notify["state"].connect (on_state_change);
      state = QuicklistControllerState.LABEL;
    }

    private void new_menu ()
    {
      menu = new QuicklistMenu () as Ctk.Menu;
      if (Unity.global_shell is Unity.Shell)
        {
          menu.destroy.connect (() => {
            Unity.global_shell.remove_fullscreen_request (this);
          });
          menu.set_swallow_clicks (Unity.global_shell.menus_swallow_events);
        }
      menu.set_detect_clicks (false);
      menu.attach_to_actor (attached_controller.child as Ctk.Actor);

      attach_to_stage ((attached_controller).child, (attached_controller).child);

      float x;
      float y;
      menu.get_position (out x, out y);
      menu.set_position (x - (float) 22.0f, y - 1.0f);
    }

    private void attach_to_stage (Clutter.Actor child, Clutter.Actor parent)
    {
      child.parent_set.disconnect (attach_to_stage);
      if (child.get_stage () is Clutter.Stage)
        {
          (child.get_stage () as Clutter.Stage).add_actor (menu);
        }
      else
        {
          child.parent_set.connect (attach_to_stage);
        }
    }

    private void on_state_change ()
    {
      if (Unity.global_shell is Unity.Shell)
        Unity.global_shell.remove_fullscreen_request (this);

      if (state == QuicklistControllerState.CLOSED) return;
      if (menu == null)
        {
          new_menu ();
          warning ("state change called on menu when menu does not exist");
        }
      if (state == QuicklistControllerState.LABEL)
        {
          // just build a menu with a label for the name
          menu.remove_all ();
          string label = attached_controller.name;

          var menuitem = new QuicklistMenuItem.with_label (label);
          menuitem.activated.connect (() => {
            state = QuicklistControllerState.CLOSED;
          });

          menu.append (menuitem, true);
        }
      else if (state == QuicklistControllerState.MENU)
        {
          if (Unity.global_shell is Unity.Shell)
            Unity.global_shell.add_fullscreen_request (this);

          menu.close_on_leave = false;
          menu.set_detect_clicks (true);
          // grab the top menu
          attached_controller.get_menu_actions ((top_menu) => {
            if (top_menu is Dbusmenu.Menuitem)
            if (top_menu.get_root ())
              {
                //returns a correct root menu
                unowned GLib.List<Dbusmenu.Menuitem> menu_items = top_menu.get_children ();
                if (menu_items != null)
                  {
                    var separator = new Unity.Launcher.QuicklistMenuSeperator ();
                    get_view ().prepend (separator, false);
                  }
                menu_items.reverse ();
                foreach (Dbusmenu.Menuitem menuitem in menu_items)
                  {
                    var view_menuitem = menu_item_from_dbusmenuitem (menuitem);
                    if (view_menuitem != null)
                      get_view ().prepend (view_menuitem, false);
                  }
                menu_items.reverse ();
              }
            else
              {
                warning ("menu given not a root item");
              }
          });

          // grab the bottom menu
          attached_controller.get_menu_navigation ((bottom_menu) => {
            if (bottom_menu is Dbusmenu.Menuitem)
              if (bottom_menu.get_root ())
                {
                  // add a separator for funsies, also because its in the spec (but mostly the fun part)
                  var separator = new Unity.Launcher.QuicklistMenuSeperator ();
                  get_view ().append (separator, false);
                  //returns a correct root menu
                  unowned GLib.List<Dbusmenu.Menuitem> menu_items = bottom_menu.get_children ();
                  foreach (Dbusmenu.Menuitem menuitem in menu_items)
                    {
                      var view_menuitem = menu_item_from_dbusmenuitem (menuitem);
                      if (view_menuitem != null)
                        {
                          get_view ().append (view_menuitem, false);
                        }
                    }
                }
              else
                {
                  warning ("menu given not a root item");
                }

          });

          float x;
          float y;
          menu.get_position (out x, out y);
          if (x > 60-22)
            menu.set_position (x - (float) 22.0f, y - 1.0f);
        }
    }

    private Ctk.MenuItem? menu_item_from_dbusmenuitem (Dbusmenu.Menuitem dbusmenuitem)
    {
      string label = "UNDEFINED";
      label = dbusmenuitem.property_get (Dbusmenu.MENUITEM_PROP_LABEL);
      string type = "label";
      string check_type = dbusmenuitem.property_get (Dbusmenu.MENUITEM_PROP_TOGGLE_TYPE);

      if (check_type == Dbusmenu.MENUITEM_TOGGLE_CHECK)
        {
          type = "check";
        }

      if (check_type == Dbusmenu.MENUITEM_TOGGLE_RADIO)
        {
          type = "radio";
        }

      if (dbusmenuitem.property_get ("type") == Dbusmenu.CLIENT_TYPES_SEPARATOR)
        {
          type = "seperator";
        }

      if (dbusmenuitem.property_get (Dbusmenu.MENUITEM_PROP_ICON_NAME) != null)
        {
          type = "stock_image";
        }

      Ctk.MenuItem menuitem;

      if (type == "label")
        {
          menuitem = new QuicklistMenuItem.with_label (label);
        }

      else if (type == "check" || type == "radio")
        {
          if (type == "check")
            menuitem = new QuicklistCheckMenuItem.with_label (label);
          else
            menuitem = new QuicklistRadioMenuItem.with_label (null, label);

          int checked = dbusmenuitem.property_get_int (Dbusmenu.MENUITEM_PROP_TOGGLE_STATE);
          if (checked == Dbusmenu.MENUITEM_TOGGLE_STATE_CHECKED)
            (menuitem as Ctk.CheckMenuItem).set_active (true);

          (menuitem as Ctk.CheckMenuItem).toggled.connect (() =>
          {
            int is_checked = ((menuitem as Ctk.CheckMenuItem).get_active ()) ?
                               Dbusmenu.MENUITEM_TOGGLE_STATE_CHECKED : Dbusmenu.MENUITEM_TOGGLE_STATE_UNCHECKED;

            dbusmenuitem.property_set_int (Dbusmenu.MENUITEM_PROP_TOGGLE_STATE, is_checked);
          });

          dbusmenuitem.property_changed.connect ((property_name, value) => {
            if (property_name == Dbusmenu.MENUITEM_PROP_TOGGLE_STATE)
              {
                int* value_weak = (int*)(value);
                (menuitem as Ctk.CheckMenuItem).set_active (
                  (*value_weak == Dbusmenu.MENUITEM_TOGGLE_STATE_CHECKED) ? true : false
                 );
              }
          });
        }

      else if (type == "seperator")
        {
          menuitem = new QuicklistMenuSeperator ();
        }

      else
        {
          warning ("not a menu item we understand (yet)");
          return null;
        }

      //connect to property changes
      dbusmenuitem.property_changed.connect ((property_name, value) => {
        if (property_name == Dbusmenu.MENUITEM_PROP_LABEL)
          {
            string* value_weak = (string*)value;
            menuitem.set_label (value_weak);
          }
        else if (property_name == Dbusmenu.MENUITEM_PROP_ENABLED)
          {
            bool* value_weak = (bool*)value;
            menuitem.set_reactive (*value_weak);
          }
      });

      menuitem.reactive = dbusmenuitem.property_get_bool (Dbusmenu.MENUITEM_PROP_ENABLED);
      menuitem.activated.connect (() => {
        dbusmenuitem.handle_event ("clicked", null, Clutter.get_current_event_time ());
      });

      menuitem.activated.connect (() => {
        Idle.add (() => {
          state = QuicklistControllerState.CLOSED;
          return false;
        });
      });

      // add more menu item types here (check, radio, image) as views are made
      // right now we can just represent all menuitems as labels

      return menuitem as Ctk.MenuItem;
    }

  }
}
