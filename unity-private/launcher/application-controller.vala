/*
 *      application-controller.vala
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

namespace Unity.Launcher
{

  public class ApplicationShortcut : Object, ShortcutItem
  {
    public string exec;
    public string name;
    public string desktop_location;

    public string get_name ()
    {
      return name;
    }

    public void activated ()
    {
      Gdk.AppLaunchContext context = new Gdk.AppLaunchContext ();
      try
        {
          var desktop_file = new KeyFile ();
          desktop_file.load_from_file (this.desktop_location, 0);
          desktop_file.set_string ("Desktop Entry", "Exec", exec);
          AppInfo appinfo = new DesktopAppInfo.from_keyfile (desktop_file);
          appinfo.create_from_commandline (this.exec, this.name, 0);
          context.set_screen (Gdk.Display.get_default ().get_default_screen ());
          context.set_timestamp (Gdk.CURRENT_TIME);

          appinfo.launch (null, context);
        }
      catch (Error e)
        {
          warning (e.message);
        }

    }
  }

  /* these are the custom shortcuts for the application controller. i may
   * move these into the ScrollerChildController base class with more abstract
   * stuff but want to wait for bamf frist (clean out liblauncher cruft
   */
  public class LibLauncherShortcut : Object, ShortcutItem
  {
    public Bamf.Application app;
    public string name;

    public string get_name ()
    {
      return _("Quit");
    }

    public void activated ()
    {
      Array<uint32> xids = app.get_xids ();
      Unity.global_shell.close_xids (xids);
    }
  }

  /* this does nothing at the moment, waiting for Launcher.Favorites
   * to impliment it (no use in doing it here as we don't get a notification in
   * liblauncher about if an applications state as a favourite has changed
   */
  public class LauncherPinningShortcut : Object, ShortcutItem
  {
    public string desktop_file {get; construct;}
    public string name {
      get {
        if (is_sticky ())
          {
            return _("Remove from Launcher");
          }
        else
          {
            return _("Keep In Launcher");
          }
      }
    }

    private string? cached_uid = null;

    private bool is_sticky ()
    {
      var favorites = Unity.Favorites.get_default ();
      foreach (string uid in favorites.get_favorites ())
        {
          if (favorites.get_string (uid, "desktop_file") == desktop_file)
            {
              cached_uid = uid;
              return true;
            }
        }
      return false;
    }

    public LauncherPinningShortcut (string _desktop_file)
    {
      Object (desktop_file: _desktop_file);
    }

    construct
    {
    }

    public string get_name ()
    {
      return name;
    }

    public void activated ()
    {
      var favorites = Unity.Favorites.get_default ();
      if (!is_sticky ())
        {
          if (cached_uid == null)
            {
              //generate a new uid
              cached_uid = "app-" + Path.get_basename (desktop_file);
            }
          favorites.set_string (cached_uid, "type", "application");
          favorites.set_string (cached_uid, "desktop_file", desktop_file);
          favorites.add_favorite (cached_uid);
        }
      else
        {
          favorites.remove_favorite (cached_uid);
        }
    }
  }
  public errordomain AppTypeError {
    NO_DESKTOP_FILE
  }

  public class ApplicationController : ScrollerChildController
  {
    public string desktop_file { get; private set; }
    
    private KeyFile desktop_keyfile;
    private string icon_name;
    private Unity.ThemeFilePath theme_file_path;
    private Bamf.Application? app = null;

    private bool is_favorite = false;

    public ApplicationController (string? desktop_file_, ScrollerChild child_)
    {
      Object (child: child_);
      
      if (desktop_file_ != null)
        {
          desktop_file = desktop_file_;
          load_desktop_file_info ();
        }
    }

    ~ApplicationController ()
    {
    }

    construct
    {
      theme_file_path = new Unity.ThemeFilePath ();
      var favorites = Unity.Favorites.get_default ();
      favorites.favorite_added.connect (on_favorite_added);
      favorites.favorite_removed.connect (on_favorite_removed);

      // we need to figure out if we are a favorite
      is_favorite = true;
      child.pin_type = PinType.UNPINNED;
      foreach (string uid in favorites.get_favorites ())
        {
          if (favorites.get_string (uid, "desktop_file") == desktop_file)
            {
              is_favorite = true;
              child.pin_type = PinType.PINNED;
              break;
            }
        }

      //connect to our quicklist signals so that we can expose and de-expose
      // windows at the correct time
      var controller = QuicklistController.get_default ();
      controller.menu_state_changed.connect ((open) => {
        if (controller.get_attached_actor () == child && app != null)
          {
            // this is a menu relating to us
            if (open)
              {
                Unity.global_shell.expose_xids (app.get_xids ());
              }
            else
              {
                Unity.global_shell.stop_expose ();
              }
          }
      });
    }

    public void set_sticky (bool is_sticky = true)
    {
      if (desktop_file == "" || desktop_file == null)
        return;
      //string uid = "app-" + Path.get_basename (desktop_file);
      var favorites = Unity.Favorites.get_default ();

      string uid = favorites.find_uid_for_desktop_file (desktop_file);

      if (is_sticky)
        {
          favorites.set_string (uid, "type", "application");
          favorites.set_string (uid, "desktop_file", desktop_file);
          favorites.add_favorite (uid);
        }
      else
        {
          favorites.remove_favorite (uid);
        }
    }

    public void close_windows ()
    {
      if (app is Bamf.Application)
        {
          Array<uint32> xids = app.get_xids ();
          Unity.global_shell.close_xids (xids);
        }
    }

    public void set_priority (float priority)
    {
      if (desktop_file == "" || desktop_file == null)
        return;

      string uid = "app-" + Path.get_basename (desktop_file);
      var favorites = Unity.Favorites.get_default ();
      favorites.set_float (uid, "priority", priority);
    }

    public float get_priority () throws AppTypeError
    {
      if (desktop_file == "" || desktop_file == null)
        throw new AppTypeError.NO_DESKTOP_FILE("There is no desktop file for this app, can't get priority");

      string uid = "app-" + Path.get_basename (desktop_file);
      var favorites = Unity.Favorites.get_default ();
      return favorites.get_float (uid, "priority");
    }

    private void on_favorite_added (string uid)
    {
      //check to see if we are the favorite
      var favorites = Unity.Favorites.get_default ();
      var desktop_filename = favorites.get_string (uid, "desktop_file");
      if (desktop_filename == desktop_file)
        {
          is_favorite = true;
          child.pin_type = PinType.PINNED;
        }
    }

    private void on_favorite_removed (string uid)
    {
      var favorites = Unity.Favorites.get_default ();
      var desktop_filename = favorites.get_string (uid, "desktop_file");
      if (desktop_filename == desktop_file)
        {
          is_favorite = false;
          child.pin_type = PinType.UNPINNED;
          closed ();
          if (".local" in desktop_filename)
           FileUtils.remove (desktop_filename);
        }
    }

    public override Gee.ArrayList<ShortcutItem> get_menu_shortcuts ()
    {
      // get the desktop file
      Gee.ArrayList<ShortcutItem> ret_list = new Gee.ArrayList<ShortcutItem> ();
      if (desktop_file == null)
        return ret_list;

      var desktop_keyfile = new KeyFile ();
      try
        {
          desktop_keyfile.load_from_file (desktop_file, 0);
        }
      catch (Error e)
        {
          warning ("Unable to load desktop file '%s': %s",
                   desktop_file,
                   e.message);
          return ret_list;
        }

      var groups = desktop_keyfile.get_groups ();
      for (int a = 0; a < groups.length; a++)
      {
        if ("QuickList Entry" in groups[a])
          {
            string exec = "";
            string name = "";
            try
              {
                exec = desktop_keyfile.get_value (groups[a], "Exec");
                name = desktop_keyfile.get_locale_string  (groups[a], "Name", "");
              } catch (Error e)
              {
                warning (e.message);
                continue;
              }
              var shortcut = new ApplicationShortcut ();
              shortcut.exec = exec;
              shortcut.name = name;
              shortcut.desktop_location = desktop_file;
              ret_list.add (shortcut);
          }
      }
      return ret_list;
    }

    public override Gee.ArrayList<ShortcutItem> get_menu_shortcut_actions ()
    {
      Gee.ArrayList<ShortcutItem> ret_list = new Gee.ArrayList<ShortcutItem> ();
      if (desktop_file != null)
        {
          var pin_entry = new LauncherPinningShortcut (desktop_file);
          ret_list.add (pin_entry);
        }

      if (app is Bamf.Application)
        {
          if (app.is_running ()) {
            var open_entry = new LibLauncherShortcut ();
            open_entry.app = app;
            ret_list.add (open_entry);
          }
        }

      return ret_list;
    }

    private static int order_app_windows (void* a, void* b)
    {
      if ((b as Bamf.Window).last_active () > (a as Bamf.Window).last_active ())
        {
          return 1;
        }
      else if ((b as Bamf.Window).last_active () == (a as Bamf.Window).last_active ())
        {
          return 0;
        }

      return -1;
    }


    public override void activate ()
    {
      if (app is Bamf.Application)
        {
          if (app.is_running ())
            {
              unowned List<Bamf.Window> windows = app.get_windows ();
              windows.sort ((CompareFunc)order_app_windows);
              Unity.global_shell.show_window (windows.nth_data (windows.length ()-1).get_xid ());
            }
        }
      else
        {
          Gdk.AppLaunchContext context = new Gdk.AppLaunchContext ();
          try
            {
              var desktop_keyfile = new KeyFile ();
              desktop_keyfile.load_from_file (desktop_file, 0);
              AppInfo appinfo = new DesktopAppInfo.from_keyfile (desktop_keyfile);
              context.set_screen (Gdk.Display.get_default ().get_default_screen ());
              context.set_timestamp (Gdk.CURRENT_TIME);

              appinfo.launch (null, context);
              child.activating = true;
              // timeout after eight seconds
              GLib.Timeout.add_seconds (8, on_launch_timeout);
            }
          catch (Error e)
            {
              warning (e.message);
            }
        }
    }

    private bool on_launch_timeout ()
    {
      child.activating = false;
      return false;
    }
    public void attach_application (Bamf.Application application)
    {
      app = application;
      desktop_file = app.get_desktop_file ();
      child.running = app.is_running ();
      child.active = app.is_active ();
      child.activating = false;

      app.running_changed.connect (on_app_running_changed);
      app.active_changed.connect (on_app_active_changed);
      app.closed.connect (detach_application);
      app.urgent_changed.connect (on_app_urgant_changed);
      name = app.get_name ();
      if (name == null || name == "")
        warning (@"Bamf returned null for app.get_name (): $desktop_file");

      icon_name = app.get_icon ();
      load_icon_from_icon_name ();
    }

    public void detach_application ()
    {
      app.running_changed.disconnect (on_app_running_changed);
      app.active_changed.disconnect (on_app_active_changed);
      app.urgent_changed.disconnect (on_app_urgant_changed);
      app.closed.disconnect (detach_application);
      app = null;
      child.running = false;
      child.active = false;
      child.needs_attention = false;
      closed ();
    }

    public bool debug_is_application_attached ()
    {
      return app != null;
    }

    private void on_app_running_changed (bool running)
    {
      child.running = running;
      if (!running)
        {
          detach_application ();
        }
    }

    private void on_app_active_changed (bool active)
    {
      child.active = active;
    }

    private void on_app_urgant_changed (bool urgancy)
    {
      child.needs_attention = urgancy;
    }

    private void load_desktop_file_info ()
    {
      try
        {
          desktop_keyfile = new KeyFile ();
          desktop_keyfile.load_from_file (desktop_file, KeyFileFlags.NONE);
        }
      catch (Error e)
        {
          warning ("could not load desktop file: %s", e.message);
        }

      try
        {
          icon_name = desktop_keyfile.get_string (KeyFileDesktop.GROUP, KeyFileDesktop.KEY_ICON);
          load_icon_from_icon_name ();
        }
      catch (Error e)
        {
          warning ("could not load icon name from desktop file: %s", e.message);
        }

      try
        {
          name = desktop_keyfile.get_string (KeyFileDesktop.GROUP, KeyFileDesktop.KEY_NAME);
        }
      catch (Error e)
        {
          warning ("could not load name from desktop file: %s", e.message);
        }
    }

    /* all this icon loading stuff can go when we switch from liblauncher to
     * bamf - please ignore any icon loading bugs :-)
     */
    private void load_icon_from_icon_name ()
    {
      // first try to load from a path;
      if (try_load_from_file (icon_name))
        {
          return;
        }

      //try to load from a path that we augment
      if (try_load_from_file ("/usr/share/pixmaps/" + icon_name))
        {
          return;
        }

      theme_file_path = new Unity.ThemeFilePath ();

      // add our searchable themes
      Gtk.IconTheme theme = Gtk.IconTheme.get_default ();
      theme_file_path.add_icon_theme (theme);
      theme = new Gtk.IconTheme ();
      theme.set_custom_theme ("unity-icon-theme");
      theme_file_path.add_icon_theme (theme);
      theme.set_custom_theme ("Web");
      theme_file_path.add_icon_theme (theme);

      theme_file_path.found_icon_path.connect ((theme, filepath) => {
        try
          {
            child.icon = new Gdk.Pixbuf.from_file (filepath);
          }
        catch (Error e)
          {
            warning (@"Could not load from $filepath");
          }
      });
      theme_file_path.failed.connect (() => {
        // we didn't get an icon, so just load the failcon
        try
          {
            var default_theme = Gtk.IconTheme.get_default ();
            child.icon = default_theme.load_icon(Gtk.STOCK_MISSING_IMAGE, 48, 0);
          }
        catch (Error e)
          {
            warning (@"Could not load any icon for %s", app.get_name ());
          }
      });

      theme_file_path.get_icon_filepath (icon_name);
    }

    private bool try_load_from_file (string filepath)
    {
      Gdk.Pixbuf pixbuf = null;
      if (FileUtils.test(filepath, FileTest.IS_REGULAR))
        {
          try
            {
              pixbuf = new Gdk.Pixbuf.from_file_at_scale(filepath,
                                                         48, 48, true);
            }
          catch (Error e)
            {
              warning ("Unable to load image from file '%s': %s",
                       filepath,
                       e.message);
            }

          if (pixbuf is Gdk.Pixbuf)
            {
              child.icon = pixbuf;
              return true;
            }
        }
      return false;
    }
  }
}
