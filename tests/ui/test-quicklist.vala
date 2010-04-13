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

using Unity;
using Unity.Testing;

namespace Unity.Tests.UI
{
  public class QuicklistSuite : Object
  {
    private const string DOMAIN = "/UI/Quicklist";

    Gtk.Window    window;
    Clutter.Actor stage;
    string        n;

    public QuicklistSuite ()
    {
      window = new Gtk.Window (Gtk.WindowType.TOPLEVEL);
      stage = Clutter.Stage.get_default ();
      n = "hello";

      Test.add_data_func (DOMAIN + "/Allocation", test_allocation);
    }

    private void test_allocation ()
    {
      debug ("test_allocation: %p %p %p %s", this, window, stage, n);
    }
  }
}
