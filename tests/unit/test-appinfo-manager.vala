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
      Test.add_data_func ("/Unit/AppInfoManager/SyncLookupOk",
                          AppInfoManagerSuite.test_sync_lookup_ok);
      Test.add_data_func ("/Unit/AppInfoManager/AsyncLookupOk",
                          AppInfoManagerSuite.test_async_lookup_ok);
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
      
      try {
        AppInfo? appinfo = yield manager.lookup_async ("_foobar.desktop");
        assert (appinfo == null);
      } catch (Error e) {
        error ("Error reading desktop file: %s", e.message);
      }
      
      mainloop.quit ();
    }
    
    /* Test that we can lookup something which is indeed there */
    internal static void test_sync_lookup_ok()
    {
      string old_datadir = Environment.get_user_data_dir ();
      var manager = AppInfoManager.get_instance();
      
      Environment.set_variable ("XDG_DATA_HOME", Config.TESTUNITDIR+"/data", true);
      debug ("XDG %s", Config.TESTUNITDIR+"/data");
      
      var info = manager.lookup ("ubuntu-about.desktop");
      assert (info != null);
      assert (info is AppInfo);
      assert ("About Ubuntu" == info.get_name ());
      
      Gee.List<string> categories = manager.get_categories ("ubuntu-about.desktop");
      assert (categories != null);
      assert (categories[0] == "GNOME");
      assert (categories[1] == "Application");
      assert (categories[2] == "Core");
      
      /* Reset the environment like a good citizen */
      Environment.set_variable ("XDG_DATA_HOME", old_datadir, true);
    }

    internal static void test_async_lookup_ok()
    {
      MainLoop mainloop = new MainLoop();
      do_test_async_lookup_ok.begin(mainloop);
      mainloop.run ();
    }
    
    internal static async void do_test_async_lookup_ok (MainLoop mainloop)
    {
      string old_datadir = Environment.get_user_data_dir ();
      var manager = AppInfoManager.get_instance();
      
      Environment.set_variable ("XDG_DATA_HOME", Config.TESTUNITDIR+"/data", true);      
      
      try{
        var info = yield manager.lookup_async ("ubuntu-about.desktop");
        assert (info is AppInfo);
        assert ("About Ubuntu" == info.get_name ());
      } catch (Error e) {
        error ("Error reading desktop file: %s", e.message);
      }
      
      Gee.List<string> categories = manager.get_categories ("ubuntu-about.desktop");
      assert (categories != null);
      assert (categories[0] == "GNOME");
      assert (categories[1] == "Application");
      assert (categories[2] == "Core");
      
      /* Reset the environment like a good citizen */
      Environment.set_variable ("XDG_DATA_HOME", old_datadir, true);
      
      mainloop.quit ();
    }
        
  }
}
