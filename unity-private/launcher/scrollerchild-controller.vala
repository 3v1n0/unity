/*
 *      scrollerchild-controller.vala
 *      Copyright (C) 2010 Canonical Ltd
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 3 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 *
 *
 *      Authored by Gordon Allott <gord.allott@canonical.com>
 */


namespace Unity.Launcher
{
  //long name for a reason, don't use this outside of the launcher :P
  public enum ScrollerChildControllerMenuState
  {
    NO_MENU,
    LABEL,
    MENU,
    MENU_CLOSE_WHEN_LEAVE
  }

  public abstract class ScrollerChildController : Object, Unity.Drag.Model
  {
    public ScrollerChild child {get; construct;}
    public signal void request_removal (); //call when not needed anymore so we can unref
    public string name = "If you can read this, file a bug!!";
    public bool hide {get; set;}


    public signal void closed ();

    protected ScrollerChildControllerMenuState menu_state;
    protected uint32 last_press_time = 0;
    protected bool button_down = false;
    protected float click_start_pos = 0.0f;
    protected int drag_sensitivity = 7;

    protected QuicklistController? menu {get; set;}

    public ScrollerChildController (ScrollerChild child_)
    {
      Object (child: child_);
    }

    construct
    {
      child.controller = this;
      child.button_press_event.connect (on_press_event);
      child.button_release_event.connect (on_release_event);
      child.enter_event.connect (on_enter_event);
      child.leave_event.connect (on_leave_event);
      child.motion_event.connect (on_motion_event);

      child.set_reactive (true);
      child.opacity = 0;
      var anim = child.animate (Clutter.AnimationMode.EASE_IN_QUAD,
                                SHORT_DELAY,
                                "opacity", 0xff);
    }

    public delegate void menu_cb (Dbusmenu.Menuitem? menu);
    public abstract void get_menu_actions (menu_cb callback);
    public abstract void get_menu_navigation (menu_cb callback);

    public abstract void activate ();
    public abstract QuicklistController get_menu_controller ();

    private bool on_leave_event (Clutter.Event event)
    {
      button_down = false;
      if (menu_state != ScrollerChildControllerMenuState.MENU)
        {
          menu_state = ScrollerChildControllerMenuState.NO_MENU;
        }

      ensure_menu_state ();
      return false;
    }

    private bool on_press_event (Clutter.Event event)
    {
      switch (event.button.button)
        {
          case 1:
            {
              last_press_time = event.button.time;
              button_down = true;
              click_start_pos = event.button.x;
            } break;
          case 3:
            {
              menu_state = ScrollerChildControllerMenuState.MENU;
              ensure_menu_state ();
            } break;
          default: break;
        }
      return false;
    }

    private bool on_release_event (Clutter.Event event)
    {
      if (event.button.button == 1 &&
          button_down == true &&
          event.button.time - last_press_time < 500)
        {
          if (menu is QuicklistController)
            {
              if (menu.state == QuicklistControllerState.LABEL ||
                  menu.state == QuicklistControllerState.MENU)
              {
                menu.state = QuicklistControllerState.CLOSED;
              }
            }

          activate ();
        }
      button_down = false;
      return false;
    }

    private bool on_enter_event (Clutter.Event event)
    {
      menu_state = ScrollerChildControllerMenuState.LABEL;
      ensure_menu_state ();
      return false;
    }

    private void ensure_menu_state ()
    {
      //no tooltips on drag

      if (Unity.Drag.Controller.get_default ().is_dragging) return;

      if (menu is QuicklistController == false)
        {
          menu = get_menu_controller ();
        }

      if (menu.state == QuicklistControllerState.MENU
          && QuicklistController.is_menu_open ()
          && QuicklistController.do_menus_match (menu))
        {
          // there is a menu open already, attach to the destroy so we can
          // re-ensure later
          QuicklistController.get_current_menu ().get_view ().destroy.connect (ensure_menu_state);
          return;
        }

      if (menu_state == ScrollerChildControllerMenuState.NO_MENU)
        {
          menu.state = QuicklistControllerState.CLOSED;
        }

      if (menu_state == ScrollerChildControllerMenuState.LABEL)
        {
          menu.state = QuicklistControllerState.LABEL;
        }

      if (menu_state == ScrollerChildControllerMenuState.MENU)
        {
          menu.state = QuicklistControllerState.MENU;
          menu_state = ScrollerChildControllerMenuState.NO_MENU;
        }
    }

    // this is for our drag handling
    public Clutter.Actor get_icon ()
    {
      return child;
    }

    public string get_drag_data ()
    {
      return name;
    }

    private bool on_motion_event (Clutter.Event event)
    {
      var drag_controller = Unity.Drag.Controller.get_default ();
      if (button_down && drag_controller.is_dragging == false)
        {
          float diff = Math.fabsf (event.motion.x - click_start_pos);
          if (diff > drag_sensitivity)
            {
              float x, y;
              child.get_transformed_position (out x, out y);
              drag_controller.start_drag (this,
                                          event.button.x - x,
                                          event.button.y - y);
              button_down = false;
              return true;
            }
        }
        return false;
    }
  }
}
