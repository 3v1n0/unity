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
   * so because the exposed API is immutable.
   *
   * To detect when any of the managed AppInfo objects changes, appears,
   * or goes away listen for the 'changed' signal.
   */
  public class AppInfoManager : Object
  {
    private static AppInfoManager singleton = null;
    
    private Map<string,AppInfo?> appinfo_by_id; /* id or path -> AppInfo */
    private Map<string,FileMonitor> monitors; /* parent uri -> monitor */
    private Map<string, Gee.List<string>> categories_by_id; /* desktop id or path -> xdg cats */
    private uchar[] buffer;
    private size_t buffer_size;
    
    private AppInfoManager ()
    {
      appinfo_by_id = new HashMap<string,AppInfo?> (GLib.str_hash, GLib.str_equal);
      categories_by_id = new HashMap<string,Gee.List<string>> (GLib.str_hash, GLib.str_equal);
      buffer_size = 1024;
      buffer = new uchar[buffer_size];
      
      monitors = new HashMap<string,AppInfo?> (GLib.str_hash, GLib.str_equal);
      foreach (string path in IO.get_system_data_dirs())
        {
          var dir = File.new_for_path (
                                 Path.build_filename (path, "applications"));
          try {            
            var monitor = dir.monitor_directory (FileMonitorFlags.NONE);
            monitor.changed.connect (on_dir_changed);
            monitors.set (dir.get_uri(), monitor);
          } catch (IOError e) {
            warning ("Error setting up directory monitor on '%s': %s",
                     dir.get_uri (), e.message);
          }
        }
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
     * Emitted whenever an AppInfo in any of the monitored paths change.
     * Note that @new_appinfo may be null in case it has been removed.
     */
    public signal void changed (string id, AppInfo? new_appinfo);
    
    /* Whenever something happens to a monitored file,
     * we remove it from the cache */
    private void on_dir_changed (FileMonitor mon, File file, File? other_file, FileMonitorEvent e)
    {
      var desktop_id = file.get_basename ();
      var path = file.get_path ();
      AppInfo? appinfo;
      
      if (appinfo_by_id.has_key(desktop_id))
        {
          appinfo_by_id.unset(desktop_id);
          appinfo = lookup (desktop_id);
          changed (desktop_id, appinfo);
        }
      
      if (appinfo_by_id.has_key(path))
        {
          appinfo_by_id.unset(path);
          appinfo = lookup (path);
          changed (path, appinfo);
        }
    }
    
    /**
     * Look up an AppInfo given its desktop id or absolute path. The desktop id
     * is the base filename of the .desktop file for the application including
     * the .desktop extension.
     *
     * If the AppInfo is not already cached this method will do synchronous
     * IO to look it up.
     */
    public AppInfo? lookup (string id)
    {
      /* Check the cache. Note that null is a legal value since it means that
       * the files doesn't exist  */
      if (appinfo_by_id.has_key (id))
        return appinfo_by_id.get (id);
    
      /* Look up by path or by desktop id */
      AppInfo? appinfo;
      KeyFile? keyfile = new KeyFile ();
      if (id.has_prefix("/"))
        {
          try {
            keyfile.load_from_file (id, KeyFileFlags.NONE);
          } catch (Error e) {
            keyfile = null;
            if (!(e is IOError.NOT_FOUND || e is KeyFileError.NOT_FOUND))
              warning ("Error loading '%s': %s", id, e.message);            
          }
          var dir = File.new_for_path (id).get_parent ();
          var dir_uri = dir.get_uri ();
          if (!monitors.has_key (dir_uri))
            {
              try {
                var monitor = dir.monitor_directory (FileMonitorFlags.NONE);
                monitor.changed.connect (on_dir_changed);
                monitors.set (dir_uri, monitor);
                debug ("Monitoring extra app directory: %s", dir_uri);
              } catch (IOError ioe) {
                warning ("Error setting up extra app directory monitor on '%s': %s",
                         dir_uri, ioe.message);
              }
            }
        }
      else
        {
          string path = Path.build_filename ("applications", id, null);
          string full_path;
          try {
            keyfile.load_from_data_dirs (path, out full_path, KeyFileFlags.NONE);
          } catch (Error e) {
            keyfile = null;
            if (!(e is IOError.NOT_FOUND || e is KeyFileError.NOT_FOUND))
                warning ("Error loading '%s': %s", id, e.message);
          }
        }
      
      /* If keyfile is null we had an error loading it */
      if (keyfile != null)
        {
          appinfo = new DesktopAppInfo.from_keyfile (keyfile);
          try {
            string[] categories = keyfile.get_string_list ("Desktop Entry",
                                                           "Categories");
            var cats = new Gee.ArrayList<string>();
            foreach (var cat in categories)
              cats.add (cat);
            
            categories_by_id.set (id, cats);
          } catch (KeyFileError e) {
            warning ("Error extracting XDG category info from '%s': %s",
                     id, e.message);
          }          
        }
      else
        appinfo = null;      

      /* If we don't find the file, we also cache that fact since we'll store
       * a null AppInfo in that case */
      appinfo_by_id.set (id, appinfo);
      
      return appinfo;
    }
    
    /**
     * Look up XDG categories for for desktop id or file path @id.
     * Returns null if id is not found.
     * This method will do sync IO if the desktop file for @id is not
     * already cached. So if you are living in an async world you must first
     * do an async call to lookup_async(id) before calling this method, in which
     * case no sync io will be done.
     */
    public Gee.List<string>? get_categories (string id)
    {
      /* Make sure we have loaded the relevant .desktop file: */
      AppInfo? appinfo = lookup (id);
      
      if (appinfo == null)
        return null;

      return categories_by_id.get (id);
    }
    
    /**
     * Look up an AppInfo given its desktop id or absolute path.
     * The desktop id is the base filename of the .desktop file for the
     * application including the .desktop extension.
     *
     * If the AppInfo is not already cached this method will do asynchronous
     * IO to look it up.
     */
    public async AppInfo? lookup_async (string id) throws Error
    {    
      /* Check the cache. Note that null is a legal value since it means that
       * the files doesn't exist  */
      if (appinfo_by_id.has_key (id))
        return appinfo_by_id.get (id);
      
      /* Load it async */            
      size_t data_size;
      void* data;
      FileInputStream input;
      
      /* Open from path or by desktop id */
      if (id.has_prefix ("/"))
        {
          var f = File.new_for_path (id);
          input = yield f.read_async (Priority.DEFAULT, null);
          var dir = f.get_parent ();
          var dir_uri = dir.get_uri ();
          if (!monitors.has_key (dir_uri))
            {
              try {
                var monitor = dir.monitor_directory (FileMonitorFlags.NONE);
                monitor.changed.connect (on_dir_changed);
                monitors.set (dir_uri, monitor);
                debug ("Monitoring extra app directory: %s", dir_uri);
              } catch (IOError ioe) {
                warning ("Error setting up extra app directory monitor on '%s': %s",
                         dir_uri, ioe.message);
              }
            }
        }
      else
        {
          string path = Path.build_filename ("applications", id, null);
          input = yield IO.open_from_data_dirs (path);
        }
      
      /* If we don't find the file, we also cache that fact by caching a
       * null value for that id  */
      if (input == null)
        {
          appinfo_by_id.set (id, null);
          return null;
        }
      
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
      var appinfo = new DesktopAppInfo.from_keyfile (keyfile);
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
