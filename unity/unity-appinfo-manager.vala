/*
 * Copyright (C) 2010 Canonical, Ltd.
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License
 * version 3.0 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License version 3.0 for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Authored by Mikkel Kamstrup Erlandsen <mikkel.kamstrup@canonical.com>
 *
 */

/*
 * IMPLEMENTATION NOTE:
 * We want the generatedd C API to be nice and not too Vala-ish. We must
 * anticipate that place daemons consuming libunity will be written in
 * both Vala and C.
 *
 */

using Gee;

namespace Unity {

  /**
   * A singleton class that caches GLib.AppInfo objects.
   * Singletons are evil, yes, but this on slightly less
   * so because the exposed API is immutable
   */
  public class AppInfoManager : Object
  {
    private static AppInfoManager singleton = null;
    
    private Map<string,AppInfo> appinfo_by_id;
    private uchar[] buffer;
    private size_t buffer_size;
    
    private AppInfoManager ()
    {
      appinfo_by_id = new HashMap<string,AppInfo> (GLib.str_hash, GLib.str_equal);
      buffer_size = 1024;
      buffer = new uchar[buffer_size];
    }
    
    /**
     * Get a ref to the singleton AppInfoManager
     */
    public static AppInfoManager get_instance ()
    {
      if (AppInfoManager.singleton == null)
        AppInfoManager.singleton = new AppInfoManager();
      
      return AppInfoManager.singleton;
    }
    
    /**
     * Look up an AppInfo given its desktop id. The desktop id is the
     * base filename of the .desktop file for the application including
     * the .desktop extension.
     *
     * If the AppInfo is not already cached this method will do synchronous
     * IO to look it up.
     */
    public AppInfo? lookup (string id)
    {
      var appinfo = appinfo_by_id.get (id);
      if (appinfo != null)
        return appinfo;
      
      appinfo = new DesktopAppInfo (id);
      if (appinfo != null)
        {
          appinfo_by_id.set (id, appinfo);
          // FIXME install monitor
        }
      return appinfo;
    }
    
    /**
     * Look up an AppInfo given its desktop id. The desktop id is the
     * base filename of the .desktop file for the application including
     * the .desktop extension.
     *
     * If the AppInfo is not already cached this method will do asynchronous
     * IO to look it up.
     */
    public async AppInfo? lookup_async (string id) throws Error
    {    
      /* Check the cache */
      var appinfo = appinfo_by_id.get (id);
      if (appinfo != null)
        {
          return appinfo;
        }
      
      /* Load it async */            
      size_t data_size;
      void* data;
      
      string path = Path.build_filename ("applications", id, null);
      FileInputStream input = yield IO.open_from_data_dirs (path);
      
      if (input == null)
        return null;
      
      try
        {
          /* Note that we must manually free 'data' */
          yield IO.read_stream_async (input, buffer, buffer_size,
                                      Priority.DEFAULT, null,
                                      out data, out data_size);
        }
      catch (Error e)
        {
          warning ("Error reading '%s': %s", id, e.message);
          return null;
        }
      
      var keyfile = new KeyFile ();
      try
        {
          keyfile.load_from_data ((string) data, data_size, KeyFileFlags.NONE);
        }
      catch (Error ee)
        {
          warning ("Error parsing '%s': %s", id, ee.message);
          return null;
        }
      
      /* Create the appinfo and cache it */
      appinfo = new DesktopAppInfo.from_keyfile (keyfile);
      appinfo_by_id.set (id, appinfo);
      
      /* Manually free the raw file data */
      g_free (data);
      
      return appinfo;
    }
    
    /* Clear all cached AppInfos */
    public void clear ()
    {
      appinfo_by_id.clear ();
    }
  
  } /* class AppInfoManager */

} /* namespace */
