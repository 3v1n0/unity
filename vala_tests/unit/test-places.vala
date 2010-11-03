/* -*- Mode: vala; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/*
 * Copyright (C) 2009 Canonical Ltd
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

namespace Unity.Tests.Unit
{
  public class PlacesSuite
  {
    public PlacesSuite ()
    {
      /*Test.add_func ("/Unit/Places/TestPlace", () => {

        Logging.init_fatal_handler ();

        var place = new TestPlace ();

        assert (place is TestPlace);

        var loop = new MainLoop (null, false);

        try
          {
            DBus.Connection conn = DBus.Bus.get (DBus.BusType.SESSION);

            Utils.register_object_on_dbus (conn,
                                    "/com/canonical/Unity/Place",
                                    place);
            loop.get_context ().iteration (true);
          }
        catch (Error e)
          {
            warning ("TestPlace error: %s", e.message);
          }
      });*/
    }

    /*public class TestPlace : Unity.Place
    {
      public TestPlace ()
      {
        Object (name:"neil", icon_name:"gtk-apply", tooltip:"hello");
      }

      construct
      {
        this.is_active.connect ((a) => {print (@"$a\n");});
      }
    }*/
  }
}
