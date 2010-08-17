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
    public static const string ICON = "/usr/share/unity/trash.png";

    public Volume volume { get; construct; }

    public VolumeChildController (Volume volume)
    {
      Object (child:new ScrollerChild (),
              volume:volume);
    }

    construct
    {
      name = volume.get_name ();
      
      var icon = volume.get_icon ();
      var icon_name = "";
      if (icon is ThemedIcon)
        {
          icon_name = (icon as ThemedIcon).get_names ()[0];
        }
      else if (icon is FileIcon)
        {
          icon_name = (icon as FileIcon).get_file ().get_path ();
        }
      else
        {
          icon_name = ICON;
        }
      load_icon_from_icon_name (icon_name);

      child.group_type = ScrollerChild.GroupType.DEVICE;

      volume.removed.connect (on_volume_removed);
    }

    private void on_volume_removed ()
    {
      ScrollerModel s;
      
      s = ObjectRegistry.get_default ().lookup ("UnityScrollerModel")[0] as ScrollerModel;
      s.remove (this.child);

      this.unref ();
    }

    /* Overides */
    public override void activate ()
    {
      try {
        Mount? mount = volume.get_mount ();
        if (mount is Mount)
          {
            Gtk.show_uri (null,
                          mount.get_root ().get_uri (),
                          Clutter.get_current_event_time ());
          }
        else
          {
            debug ("No mount point");
          }

      } catch (Error e) {
        warning (@"Unable to show Trash: $(e.message)");
      }
    }

    public override QuicklistController? get_menu_controller ()
    {
      return new ApplicationQuicklistController (this);
    }

    public override void get_menu_actions (ScrollerChildController.menu_cb callback)
    {
      Dbusmenu.Menuitem root = new Dbusmenu.Menuitem ();
      root.set_root (true);
     
      callback (root);
    }

    public override void get_menu_navigation (ScrollerChildController.menu_cb callback)
    {
      Dbusmenu.Menuitem root = new Dbusmenu.Menuitem ();
      root.set_root (true);

      Dbusmenu.Menuitem item;

      item = new Dbusmenu.Menuitem ();
      item.property_set (Dbusmenu.MENUITEM_PROP_LABEL, "Open");
      item.property_set_bool (Dbusmenu.MENUITEM_PROP_ENABLED, true);
      item.property_set_bool (Dbusmenu.MENUITEM_PROP_VISIBLE, true);
      root.child_append (item);
      item.item_activated.connect (() => {
        try {
          Gtk.show_uri (null, "trash://", Clutter.get_current_event_time ());
        } catch (Error e) {
          warning (@"Unable to show Trash: $(e.message)");
        }
      });

      callback (root);
    }

    public override bool can_drag ()
    {
      return true;
    }
  }
}
