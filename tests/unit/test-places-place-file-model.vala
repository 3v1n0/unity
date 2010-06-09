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
using Unity.Places;

namespace Unity.Tests.Unit
{
  public class PlacesPlaceFileModel : Object
  {
    public const string DOMAIN = "/Unit/Places/PlaceFileModel";

    public PlacesPlaceFileModel ()
    {
      Logging.init_fatal_handler ();

      Test.add_data_func (DOMAIN + "/Allocation", test_allocation);
    }

    private void test_allocation ()
    {
      var model = new PlaceFileModel.with_directory (TESTDIR+"/data");
      assert (model is PlaceFileModel);
      assert (model.size == 2);
    }
  }
}
