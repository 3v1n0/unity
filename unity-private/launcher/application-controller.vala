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
      } catch (Error e)
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
    public LibLauncher.Application app;
    public string name;

    public string get_name ()
    {
      return _("Quit");
    }

    public void activated ()
    {
      this.app.close (Clutter.get_current_event_time ());
    }
  }

  /* this does nothing at the moment, waiting for Launcher.Favorites
   * to impliment it (no use in doing it here as we don't get a notification in
   * liblauncher about if an applications state as a favourite has changed
   */
  public class LauncherPinningShortcut : Object, ShortcutItem
  {
    public string uid {get; construct;}

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

    private bool is_sticky ()
    {
      var favorites = LibLauncher.Favorites.get_default ();
      unowned SList<string> favorite_list = favorites.get_favorites();
      foreach (weak string favuid in favorite_list)
        {
          // we only want favorite *applications* for the moment
          var type = favorites.get_string(favuid, "type");
          if (type != "application")
              continue;

          if (uid == favuid)
            return true;
        }
      return false;
    }


    public LauncherPinningShortcut (string _uid)
    {
      Object (uid: _uid);
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
      debug ("launcher pinning shortcut called");
    }
  }

  class ApplicationController : ScrollerChildController
  {
    private string _desktop_file;
    public string desktop_file {
      get
        {
          return _desktop_file;
        }
      set
        {
          _desktop_file = value;
          load_desktop_file_info ();
        }
    }
    private KeyFile desktop_keyfile;
    private string icon_name;
    private Unity.ThemeFilePath theme_file_path;
    private LibLauncher.Application app;

    public ApplicationController (string desktop_file_, ScrollerChild child_)
    {
      Object (desktop_file: desktop_file_,
              child: child_);
    }

    construct
    {
      theme_file_path = new Unity.ThemeFilePath ();
    }

    public override Gee.ArrayList<ShortcutItem> get_menu_shortcuts ()
    {
      // get the desktop file
      Gee.ArrayList<ShortcutItem> ret_list = new Gee.ArrayList<ShortcutItem> ();
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
/*
      if (!this.readonly && !this.is_fixed)
        {
          var pin_entry = new LauncherPinningShortcut (this);
          if (this.app != null && this.app.get_desktop_file () != null)
            ret_list.add (pin_entry);
        }
*/

      if (app is LibLauncher.Application)
        {
          if (app.running) {
            var open_entry = new LibLauncherShortcut ();
            open_entry.app = app;
            ret_list.add (open_entry);
          }
        }

      return ret_list;
    }

    public override void activate ()
    {
      if (app is LibLauncher.Application)
        {
          if (app.running)
            {
              if (!app.focused)
                {
                  app.show (Clutter.get_current_event_time ());
                  return;
                }
            }
        }
      Gdk.AppLaunchContext context = new Gdk.AppLaunchContext ();
      try
      {
        var desktop_keyfile = new KeyFile ();
        desktop_keyfile.load_from_file (desktop_file, 0);
        AppInfo appinfo = new DesktopAppInfo.from_keyfile (desktop_keyfile);
        context.set_screen (Gdk.Display.get_default ().get_default_screen ());
        context.set_timestamp (Gdk.CURRENT_TIME);

        appinfo.launch (null, context);
      } catch (Error e)
      {
        warning (e.message);
      }


    }

    public void attach_application (LibLauncher.Application application)
    {
      app = application;
      child.running = app.running;
      child.active = app.focused;
      app.running_changed.connect (() => {
        child.running = app.running;
      });

      app.focus_changed.connect (() => {
        child.active = child.focused;
      });
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
            warning (@"Could not load any icon for %s", app.name);
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
