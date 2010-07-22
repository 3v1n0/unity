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
    private Dbusmenu.Client menu_client;
    private Dbusmenu.Menuitem cached_menu;
    private int menu_items_realized_counter;

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

      notify["menu"].connect (on_notify_menu);
    }

    public override QuicklistController get_menu_controller ()
    {
      QuicklistController new_menu = new ApplicationQuicklistController (this);
      return new_menu;
    }

    private void on_notify_menu ()
    {
      menu.notify["state"].connect (() => {
        if (menu.state == QuicklistControllerState.MENU)
          {
            Unity.global_shell.expose_xids (app.get_xids ());
          }
        else
          {
            Unity.global_shell.stop_expose ();
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

    public bool is_sticky ()
    {
      if (desktop_file == "" || desktop_file == null)
        return false;

      var favorites = Unity.Favorites.get_default ();
      string uid = favorites.find_uid_for_desktop_file (desktop_file);
      if (uid != null && uid != "")
        return true;
      else
        return false;
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

    public override void get_menu_actions (ScrollerChildController.menu_cb callback)
    {

      // first check to see if we have a cached client, if we do, just re-use that
      if (menu_client is Dbusmenu.Client && cached_menu is Dbusmenu.Menuitem)
        {
          callback (cached_menu);
        }

      // check for a menu from bamf
      if (app is Bamf.Application)
        {
          GLib.List<Bamf.View> views = app.get_children ();
          foreach (Bamf.View view in views)
            {
              if (view is Bamf.Indicator)
                {
                  string path = (view as Bamf.Indicator).get_dbus_menu_path ();
                  string remote_address = (view as Bamf.Indicator).get_remote_address ();
                  string remote_path = (view as Bamf.Indicator).get_remote_path ();

                  // Yes, right here, i have lambda's inside lambda's... shutup.
                  menu_client = new Dbusmenu.Client (remote_address, path);
                  menu_client.layout_updated.connect (() => {
                    var menu = menu_client.get_root ();
                    cached_menu = menu;
                    if (menu is Dbusmenu.Menuitem == false)
                      warning (@"Didn't get a menu for path: $path - address: $remote_address");

                    unowned GLib.List<Dbusmenu.Menuitem> menu_items = menu.get_children ();
                    menu_items_realized_counter = (int)menu_items.length ();
                    foreach (Dbusmenu.Menuitem menuitem in menu_items)
                    {
                      menuitem.realized.connect (() =>
                        {
                          menu_items_realized_counter -= 1;
                          if (menu_items_realized_counter < 1)
                            {
                              callback (menu);
                            }

                        });
                    }
                  });
                }
            }
        }


      if (desktop_file == "" || desktop_file == null)
        {
          callback (null);
        }

      // find our desktop shortcuts
      Indicator.DesktopShortcuts shortcuts = new Indicator.DesktopShortcuts (desktop_file, "Unity");
      unowned string [] nicks = shortcuts.get_nicks ();

      if (nicks.length < 1)
        callback (null);

      Dbusmenu.Menuitem root = new Dbusmenu.Menuitem ();
      root.set_root (true);

      foreach (string nick in nicks)
        {
          string local_nick = nick.dup ();
          unowned string name = shortcuts.nick_get_name (local_nick);
          string local_name = name.dup ();

          Dbusmenu.Menuitem shortcut_item = new Dbusmenu.Menuitem ();
          shortcut_item.property_set (Dbusmenu.MENUITEM_PROP_LABEL, local_name);
          shortcut_item.property_set_bool (Dbusmenu.MENUITEM_PROP_ENABLED, true);
          shortcut_item.property_set_bool (Dbusmenu.MENUITEM_PROP_VISIBLE, true);
          shortcut_item.item_activated.connect ((timestamp) => {
            shortcuts.nick_exec (local_nick);
          });
          root.child_append (shortcut_item);

        }
      callback (root);
    }


    public override void get_menu_navigation (ScrollerChildController.menu_cb callback)
    {

      // build a dbusmenu that represents our generic application handling items
      Dbusmenu.Menuitem root = new Dbusmenu.Menuitem ();
      root.set_root (true);

      if (desktop_file != null)
        {
          Dbusmenu.Menuitem pinning_item = new Dbusmenu.Menuitem ();
          if (is_sticky ())
            pinning_item.property_set (Dbusmenu.MENUITEM_PROP_LABEL, _("Remove from launcher"));
          else
            pinning_item.property_set (Dbusmenu.MENUITEM_PROP_LABEL, _("Add to launcher"));

          pinning_item.property_set_bool (Dbusmenu.MENUITEM_PROP_ENABLED, true);
          pinning_item.property_set_bool (Dbusmenu.MENUITEM_PROP_VISIBLE, true);
          pinning_item.item_activated.connect ((timestamp) => {
            set_sticky (!is_sticky ());
          });

          root.child_append (pinning_item);
        }

      if (app is Bamf.Application)
        {
          Dbusmenu.Menuitem app_item = new Dbusmenu.Menuitem ();
          app_item.property_set (Dbusmenu.MENUITEM_PROP_LABEL, _("Quit"));
          app_item.property_set_bool (Dbusmenu.MENUITEM_PROP_ENABLED, true);
          app_item.property_set_bool (Dbusmenu.MENUITEM_PROP_VISIBLE, true);

          app_item.item_activated.connect ((timestamp) => {
            if (app is Bamf.Application)
              {
                Array<uint32> xids = app.get_xids ();
                Unity.global_shell.close_xids (xids);
              }
          });
          root.child_append (app_item);
        }

        callback (root);
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
      global_shell.hide_unity ();

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

      try
        {
          name = desktop_keyfile.get_string (KeyFileDesktop.GROUP, "X-GNOME-FullName");
        }
      catch (Error e)
        {
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
