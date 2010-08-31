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
 * Authored by Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */
 
using Unity.Launcher;
using Unity.Testing;
 
namespace Unity.Places
{
  public class VolumeChildController : ScrollerChildController
  {
    public static const string ICON = "/usr/share/unity/devices.png";

    public Volume volume { get; construct; }

    public VolumeChildController (Volume volume)
    {
      Object (child:new ScrollerChild (),
              volume:volume);
    }

    construct
    {
      name = volume.get_name ();
      load_icon_from_icon_name (ICON);

      child.group_type = ScrollerChild.GroupType.DEVICE;
      child.drag_removed.connect (eject_volume);

      volume.removed.connect (on_volume_removed);
    }

    private void on_volume_removed ()
    {
      ScrollerModel s;
      
      s = ObjectRegistry.get_default ().lookup ("UnityScrollerModel")[0] as ScrollerModel;
      s.remove (this.child);

      this.unref ();
    }

    private void open_volume ()
    {
      Mount? mount = volume.get_mount ();
      string error_msg = @"Cannot open volume '$(volume.get_name ())': ";

      if (mount is Mount)
        {
          var loc = mount.get_root ();
          try {
            AppInfo.launch_default_for_uri (loc.get_uri (), null);
          } catch (Error err) {
            warning (error_msg + err.message);
          }
        }
      else
        {
          if (volume.can_mount () == false)
            {
              warning (error_msg + "Cannot be mounted");
              return;
            }
          try {
            volume.mount.begin (0, null, null);

            mount = volume.get_mount ();
            if (mount is Mount)
              AppInfo.launch_default_for_uri (mount.get_root ().get_uri (), null);
            else
              warning (error_msg + "Unable to mount");

          } catch (Error e) {
            warning (error_msg +  e.message);
          }

        }
    }

    private void eject_volume ()
    {
      /*
       * Because there are bugs in making this work through the vala 0.9.5 and
       * I don't have time to debug them.
       */
      Utils.volume_eject (volume);
    }

    /* Overides */
    public override void activate ()
    {
      open_volume ();
    }

    public override QuicklistController? get_menu_controller ()
    {
      return new ApplicationQuicklistController (this);
    }

    public override void get_menu_actions (ScrollerChildController.menu_cb callback)
    {
      Dbusmenu.Menuitem root = new Dbusmenu.Menuitem ();
      root.set_root (true);
      
      Dbusmenu.Menuitem item;

      item = new Dbusmenu.Menuitem ();
      item.property_set (Dbusmenu.MENUITEM_PROP_LABEL, _("Open"));
      item.property_set_bool (Dbusmenu.MENUITEM_PROP_ENABLED, true);
      item.property_set_bool (Dbusmenu.MENUITEM_PROP_VISIBLE, true);
      item.item_activated.connect (open_volume);
      root.child_append (item);

      callback (root);
    }

    public override void get_menu_navigation (ScrollerChildController.menu_cb callback)
    {
      Dbusmenu.Menuitem root = new Dbusmenu.Menuitem ();
      root.set_root (true);

      if (volume.get_mount () != null)
        {
          Dbusmenu.Menuitem item;

          item = new Dbusmenu.Menuitem ();
          item.property_set (Dbusmenu.MENUITEM_PROP_LABEL, _("Eject"));
          item.property_set_bool (Dbusmenu.MENUITEM_PROP_ENABLED, true);
          item.property_set_bool (Dbusmenu.MENUITEM_PROP_VISIBLE, true);
          root.child_append (item);
          item.item_activated.connect (eject_volume);
        }

      callback (root);
    }

    public override bool can_drag ()
    {
      return true;
    }
  }
}
