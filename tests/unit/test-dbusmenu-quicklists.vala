/*
 *      test-launcher.vala
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
using Unity;
using Unity.Launcher;
using Unity.Testing;

/* TODO - add more tests once we have bamf and can test the controllers */
namespace Unity.Tests.Unit
{
  public class TestScrollerChildController : ScrollerChildController
  {
    public TestScrollerChildController (ScrollerChild child_)
    {
      Object (child: child_);
    }

    construct
    {
      name = "Testing";
    }

    public override void get_menu_actions (ScrollerChildController.menu_cb callback)
    {
      // our actions menu goes as follows
      // + Root
      //  - Checked Menu Item, unchecked, "Unchecked"
      //  - Checked Menu Item, checked, "Checked"
      //  - Separator
      //  - Label "A Label"
      //  - Radio Menu Item, checked, "Radio Active"
      //  - Radio Menu Item, unchecked, "Radio Unactive"
      Dbusmenu.Menuitem root = new Dbusmenu.Menuitem ();
      root.set_root (true);

      Dbusmenu.Menuitem menuitem = new Dbusmenu.Menuitem ();
      menuitem.property_set (Dbusmenu.MENUITEM_PROP_LABEL, "Unchecked");
      menuitem.property_set (Dbusmenu.MENUITEM_PROP_TOGGLE_TYPE, Dbusmenu.MENUITEM_TOGGLE_CHECK);
      menuitem.property_set_bool (Dbusmenu.MENUITEM_PROP_ENABLED, true);
      menuitem.property_set_bool (Dbusmenu.MENUITEM_PROP_VISIBLE, true);
      menuitem.property_set_int (Dbusmenu.MENUITEM_PROP_TOGGLE_STATE, Dbusmenu.MENUITEM_TOGGLE_STATE_UNCHECKED);
      root.child_append (menuitem);

      menuitem = new Dbusmenu.Menuitem ();
      menuitem.property_set (Dbusmenu.MENUITEM_PROP_LABEL, "Checked");
      menuitem.property_set (Dbusmenu.MENUITEM_PROP_TOGGLE_TYPE, Dbusmenu.MENUITEM_TOGGLE_CHECK);
      menuitem.property_set_bool (Dbusmenu.MENUITEM_PROP_ENABLED, true);
      menuitem.property_set_bool (Dbusmenu.MENUITEM_PROP_VISIBLE, true);
      menuitem.property_set_int (Dbusmenu.MENUITEM_PROP_TOGGLE_STATE, Dbusmenu.MENUITEM_TOGGLE_STATE_CHECKED);
      root.child_append (menuitem);

      menuitem = new Dbusmenu.Menuitem ();
      menuitem.property_set ("type", Dbusmenu.CLIENT_TYPES_SEPARATOR);
      menuitem.property_set_bool (Dbusmenu.MENUITEM_PROP_ENABLED, true);
      menuitem.property_set_bool (Dbusmenu.MENUITEM_PROP_VISIBLE, true);
      root.child_append (menuitem);

      menuitem = new Dbusmenu.Menuitem ();
      menuitem.property_set (Dbusmenu.MENUITEM_PROP_LABEL, "A Label");
      menuitem.property_set_bool (Dbusmenu.MENUITEM_PROP_ENABLED, true);
      menuitem.property_set_bool (Dbusmenu.MENUITEM_PROP_VISIBLE, true);
      root.child_append (menuitem);

      menuitem = new Dbusmenu.Menuitem ();
      menuitem.property_set (Dbusmenu.MENUITEM_PROP_LABEL, "Radio Active");
      menuitem.property_set (Dbusmenu.MENUITEM_PROP_TOGGLE_TYPE, Dbusmenu.MENUITEM_TOGGLE_RADIO);
      menuitem.property_set_bool (Dbusmenu.MENUITEM_PROP_ENABLED, true);
      menuitem.property_set_bool (Dbusmenu.MENUITEM_PROP_VISIBLE, true);
      menuitem.property_set_int (Dbusmenu.MENUITEM_PROP_TOGGLE_STATE, Dbusmenu.MENUITEM_TOGGLE_STATE_CHECKED);
      root.child_append (menuitem);

      menuitem = new Dbusmenu.Menuitem ();
      menuitem.property_set (Dbusmenu.MENUITEM_PROP_LABEL, "Radio Unactive");
      menuitem.property_set (Dbusmenu.MENUITEM_PROP_TOGGLE_TYPE, Dbusmenu.MENUITEM_TOGGLE_RADIO);
      menuitem.property_set_bool (Dbusmenu.MENUITEM_PROP_ENABLED, true);
      menuitem.property_set_bool (Dbusmenu.MENUITEM_PROP_VISIBLE, true);
      menuitem.property_set_int (Dbusmenu.MENUITEM_PROP_TOGGLE_STATE, Dbusmenu.MENUITEM_TOGGLE_STATE_UNCHECKED);
      root.child_append (menuitem);

      callback (root);
    }

    public override void get_menu_navigation (ScrollerChildController.menu_cb callback)
    {
      // just tests that the order is correct
      Dbusmenu.Menuitem root = new Dbusmenu.Menuitem ();
      root.set_root (true);

      Dbusmenu.Menuitem menuitem = new Dbusmenu.Menuitem ();
      menuitem.property_set (Dbusmenu.MENUITEM_PROP_LABEL, "1");
      menuitem.property_set_bool (Dbusmenu.MENUITEM_PROP_ENABLED, true);
      menuitem.property_set_bool (Dbusmenu.MENUITEM_PROP_VISIBLE, true);
      root.child_append (menuitem);

      menuitem = new Dbusmenu.Menuitem ();
      menuitem.property_set (Dbusmenu.MENUITEM_PROP_LABEL, "2");
      menuitem.property_set_bool (Dbusmenu.MENUITEM_PROP_ENABLED, true);
      menuitem.property_set_bool (Dbusmenu.MENUITEM_PROP_VISIBLE, true);
      root.child_append (menuitem);

      menuitem = new Dbusmenu.Menuitem ();
      menuitem.property_set (Dbusmenu.MENUITEM_PROP_LABEL, "3");
      menuitem.property_set_bool (Dbusmenu.MENUITEM_PROP_ENABLED, true);
      menuitem.property_set_bool (Dbusmenu.MENUITEM_PROP_VISIBLE, true);
      root.child_append (menuitem);

      callback (root);
    }

    public override void activate ()
    {
    }

    public override QuicklistController get_menu_controller ()
    {
      QuicklistController new_menu = new ApplicationQuicklistController (this);
      return new_menu;
    }
  }

  public class QuicklistSuite
  {
    public QuicklistSuite ()
    {
      Test.add_data_func ("/Unity/Launcher/TestDbusMenu", test_dbus_menu);
    }

    private void test_dbus_menu()
    {
      ScrollerChild child = new TestScrollerChild ();
      TestScrollerChildController controller = new TestScrollerChildController(child);
      ApplicationQuicklistController menu = controller.get_menu_controller () as ApplicationQuicklistController;
      menu.state = QuicklistControllerState.LABEL;

      unowned GLib.List<Ctk.MenuItem> menuitems = null;
      assert (menuitems.length () == 0);

      menu.state = QuicklistControllerState.MENU;

      // this assert here will only work for our local dbusmenu's because they return
      // immediately. remote dbusmenu's need to be slightly more clever
      menuitems = menu.get_view ().get_items ();
      assert (menuitems.length () >= 10);

      assert (menuitems.data is Ctk.CheckMenuItem);
      assert ((menuitems.data as Ctk.CheckMenuItem).active == false);
      menuitems = menuitems.next;

      assert (menuitems.data is Ctk.CheckMenuItem);
      assert ((menuitems.data as Ctk.CheckMenuItem).active == true);
      menuitems = menuitems.next;

      assert (menuitems.data is Ctk.MenuSeperator);
      menuitems = menuitems.next;

      assert (menuitems.data is Ctk.MenuItem);
      assert ((menuitems.data as Ctk.MenuItem).label == "A Label");
      menuitems = menuitems.next;

      assert (menuitems.data is Ctk.RadioMenuItem);
      assert ((menuitems.data as Ctk.RadioMenuItem).active == true);
      menuitems = menuitems.next;

      assert (menuitems.data is Ctk.RadioMenuItem);
      assert ((menuitems.data as Ctk.RadioMenuItem).active == true);
      menuitems = menuitems.next;
    }
  }
}
