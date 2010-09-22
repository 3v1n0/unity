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

namespace Unity.Places
{

  public class PlaceEntryScrollerChildController : ScrollerChildController
  {
    public PlaceEntry entry { get; construct; }

    public signal void clicked (uint section_id);

    public PlaceEntryScrollerChildController (PlaceEntry entry)
    {
      Object (child:new ScrollerChild (), entry:entry);
    }

    construct
    {
      name = entry.name;
      load_icon_from_icon_name (entry.icon);

      entry.notify["active"].connect (() => {
        child.active = entry.active;
      });

      child.group_type = ScrollerChild.GroupType.PLACE;
      child.motion_event.connect (get_sections);
    }

    private bool get_sections ()
    {
      Dee.Model sections;

      /* We do this so the sections model actually populates with something
       * before we show it
       */
      sections = entry.sections_model;

      child.motion_event.disconnect (get_sections);

      return false;
    }

    public override void activate ()
    {
      clicked (0);
    }

    public override QuicklistController? get_menu_controller ()
    {
      return new ApplicationQuicklistController (this);
    }

    public override void get_menu_actions (ScrollerChildController.menu_cb callback)
    {
      Dbusmenu.Menuitem root = new Dbusmenu.Menuitem ();
      root.set_root (true);

      Dee.Model             sections = entry.sections_model;
      unowned Dee.ModelIter iter = sections.get_first_iter ();

      while (iter != null && !sections.is_last (iter))
        {
          var name = sections.get_string (iter, 0);
          var item = new Dbusmenu.Menuitem ();

          item.property_set (Dbusmenu.MENUITEM_PROP_LABEL, name);
          item.property_set_bool (Dbusmenu.MENUITEM_PROP_ENABLED, true);
          item.property_set_bool (Dbusmenu.MENUITEM_PROP_VISIBLE, true);
          item.property_set_int ("section-id", sections.get_position (iter));
          item.item_activated.connect ((timestamp) => {
            clicked (item.property_get_int ("section-id"));
          });
          root.child_append (item);

          iter = sections.next (iter);
        }

      callback (root);
    }

    public override bool can_drag ()
    {
      return false;
    }
  }
}
