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

namespace Unity.Places
{
  /**
   * Represents a PlaceEntry through a .place file ("offline") and then through
   * DBus ("online").
   **/
  public class PlaceEntry : Object
  {
    /* Properties */
    public string? dbus_name   { get; construct; }
    public string? dbus_path   { get; construct; }
    public string  icon        { get; construct set; }
    public string  name        { get; construct set; }
    public string  description { get; construct set; }
    public bool    show_global { get; construct set; }
    public bool    show_entry  { get; construct set; }

    /* Whether the Entry is available on the bus, this is only when we want to
     * do optimisations for startup (showing the entries before actually
     * starting the daemon. We can also abuse this for testing :)
     */
    public bool   online    { get; construct set; }

    public PlaceEntry (string dbus_name, string dbus_path)
    {
      Object (dbus_name:dbus_name, dbus_path:dbus_path);
    }

    public PlaceEntry.with_info (string dbus_name,
                                 string dbus_path,
                                 string name,
                                 string icon,
                                 string description,
                                 bool   show_global,
                                 bool   show_entry)
    {
      Object (dbus_name:dbus_name,
              dbus_path:dbus_path,
              name:name,
              icon:icon,
              description:description,
              show_global:show_global,
              show_entry:show_entry);
    }

    construct
    {
      online = false;
    }
  }
}

