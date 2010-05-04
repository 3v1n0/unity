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
  public class LauncherSuite
  {
    public LauncherSuite ()
    {
      Test.add_data_func ("/Unity/Launcher/TestScrollerModel", test_scroller_model);
    }

    public class TestScrollerChild : ScrollerChild
    {
    }

    // very basic tests for scroller model, makes sure its list interface works
    // as expected
    private void test_scroller_model ()
    {
      ScrollerModel model = new ScrollerModel ();
      ScrollerChild child_a = new TestScrollerChild ();
      ScrollerChild child_b = new TestScrollerChild ();
      ScrollerChild child_c = new TestScrollerChild ();

      model.add (child_a);
      model.add (child_c);
      model.insert (child_b, 1);

      assert (model[1] == child_b);
      assert (model[2] == child_c);

      foreach (ScrollerChild child in model)
      {
        assert (child is ScrollerChild);
      }

      assert (child_a in model);
      model.remove (child_b);
      assert (model[1] == child_c);
    }
  }
}
