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

    public signal void clicked ();

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
    }
    
    public override void activate ()
    {
      clicked ();
    }

    public override QuicklistController? get_menu_controller ()
    {
      /*FIXME: Setting the menu stops the activate handler from being called.*/
      return new ApplicationQuicklistController (this);
      //return null;
    }
  }
}
