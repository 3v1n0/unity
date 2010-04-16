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
  public interface PlaceView : Clutter.Actor
  {
    /**
     * This interface should be implemented by any view that represents a
     * place. This allows the place to request a view over dbus by the
     * (hopefully) unique name of the view, together with a bunch of properties
     * that it wants to set on the view (to basically initialise the view).
     *
     * The interface is very, very small, but it needs to be done so we can, in
     * the future, load in views from dynamic modules installed by the places
     **/

    public abstract void init_with_properties (HashTable<string, string> props);
  }
}

