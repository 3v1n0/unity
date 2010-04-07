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

using Gee;

namespace Unity.Testing
{
  public class ObjectRegistry
  {
    private static ObjectRegistry _registry = null;

    private HashMap<string,Object> object_map;

    public ObjectRegistry ()
    {
      object_map = new HashMap<string, Object> (str_hash, str_equal, direct_equal);
    }

    public static ObjectRegistry get_default ()
    {
      if (_registry == null)
        _registry = new ObjectRegistry ();

      return _registry;
    }

    public void register (string name, Object object)
    {
      object_map[name] = object;
    }

    public Object? lookup (string name)
    {
      return object_map[name];
    }
  }
}
