/* -*- Mode: vala; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
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
 * Authored by Mikkel Kamstrup Erlandsen <mikkel.kamstrup@canonical.com>
 *
 */
using Unity;
using Unity.Place;
using Unity.Testing;

namespace Unity.Tests.Unit
{
  public class AppInfoManagerSuite
  {
    
    public AppInfoManagerSuite ()
    {
      Test.add_data_func ("/Unit/AppInfoManager/Allocation",
                          AppInfoManagerSuite.test_allocation);
      Test.add_data_func ("/Unit/AppInfoManager/ClearEmpty",
                          AppInfoManagerSuite.test_clear_empty);
      Test.add_data_func ("/Unit/AppInfoManager/SyncLookupMissing",
                          AppInfoManagerSuite.test_sync_lookup_missing);
      Test.add_data_func ("/Unit/AppInfoManager/AsyncLookupMissing",
                          AppInfoManagerSuite.test_async_lookup_missing);
    }

    /* Test that we can even get a valid ref to the manager */
    internal static void test_allocation()
    {
      var manager = AppInfoManager.get_instance();
      assert (manager is AppInfoManager);
    }

    /* Test that we can clear an empty manager */
    internal static void test_clear_empty()
    {
      var manager = AppInfoManager.get_instance();
      manager.clear ();
      manager.clear ();
    }

    /* Test that we can clear an empty manager */
    internal static void test_sync_lookup_missing()
    {
      var manager = AppInfoManager.get_instance();
      assert (manager.lookup ("_foobar.desktop") == null);
    }

    internal static void test_async_lookup_missing()
    {
      MainLoop mainloop = new MainLoop();
      do_test_async_lookup_missing.begin(mainloop);
      mainloop.run ();
    }
    
    internal static async void do_test_async_lookup_missing (MainLoop mainloop)
    {
      var manager = AppInfoManager.get_instance();
      
      AppInfo? appinfo = yield manager.lookup_async ("_foobar.desktop");
      assert (appinfo == null);
      
      mainloop.quit ();
    }
        
  }
}
