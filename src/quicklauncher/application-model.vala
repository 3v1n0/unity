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
 * Authored by Gordon Allott <gord.allott@canonical.com>
 *
 */

namespace Unity.Quicklauncher.Models
{
  public class ApplicationShortcut : Object, ShortcutItem
  {
    public string exec;
    public string name;
    public string desktop_location;

    public string get_name ()
    {
      return this.name;
    }

    public void activated ()
    {
      debug ("application shortcut activated called");
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

  public class LibLauncherShortcut : Object, ShortcutItem
  {
    public Launcher.Application app;
    public string name;

    public string get_name ()
    {
      return _("Quit");
    }

    public void activated ()
    {
      debug ("liblauncher shortcut activated called");
      this.app.close (Clutter.get_current_event_time ());
    }
  }

  public class LauncherPinningShortcut : Object, ShortcutItem
  {
    public ApplicationModel app_model {get; construct;}
    public string name {
      get {
        if (this.app_model.is_sticky)
          {
            return _("Remove from Launcher");
          }
        else
          {
            return _("Keep In Launcher");
          }
      }
    }

    public LauncherPinningShortcut (ApplicationModel model)
      {
        Object (app_model: model);
      }

    construct
      {

      }

    public string get_name ()
    {
      return this.name;
    }

    public void activated ()
    {
      debug ("launcher pinning shortcut called");
      this.app_model.is_sticky = !this.app_model.is_sticky;
    }
  }

  public class ApplicationModel : Object, LauncherModel
  {
    public signal void windows_changed ();

    private Gdk.Pixbuf _icon;
    private string desktop_uri;
    private bool queued_save_priority = false;
    private bool _do_shadow = false;
    private float _priority;

    private ThemeFilePath theme_file_path;
    private string icon_file_path;

    public float priority {
      get { return _priority; }
      set { _priority = value; this.do_save_priority ();}
    }
    public bool do_shadow {
      get { return this._do_shadow; }
    }

    public string uid {
      get { return this.desktop_uri; }
    }

    public unowned SList<Wnck.Window> windows {
      get
        {
          return app.get_windows ();
        }
    }

    private Launcher.Application _app;
    public Launcher.Application app {
      get { return _app; }
      private set {
        if (_app != null)
          {
            _app.opened.disconnect(this.on_app_opened);
            _app.closed.disconnect(this.on_app_closed);
            _app.focus_changed.disconnect (this.on_app_focus_changed);
            _app.running_changed.disconnect (this.on_app_running_changed);
            _app.urgent_changed.disconnect (this.on_app_urgent_changed);
            _app.icon_changed.disconnect (this.on_app_icon_changed);
          }

        _app = value;
        _app.opened.connect(this.on_app_opened);
        _app.closed.connect(this.on_app_closed);
        _app.focus_changed.connect (this.on_app_focus_changed);
        _app.running_changed.connect (this.on_app_running_changed);
        _app.urgent_changed.connect (this.on_app_urgent_changed);
        _app.icon_changed.connect (this.on_app_icon_changed);
      }
    }

    public ApplicationModel (Launcher.Application application)
    {
      string process_name = "Model-" + application.name;
      this.app = application;
      this.desktop_uri = app.get_desktop_file ();

      string gconf_process_name = "gconf-grabbing-" + process_name;
      LOGGER_START_PROCESS (gconf_process_name);
      string uid = get_fav_uid ();
      this._is_sticky = (uid != "");
      this.grab_priority ();
      LOGGER_END_PROCESS (gconf_process_name);

      string fav_process_name = "favorite-grabbing-" + process_name;
      LOGGER_START_PROCESS (fav_process_name);
      var favorites = Launcher.Favorites.get_default ();
      if (uid != "")
        {
          this._do_shadow = favorites.get_bool (uid, "enable_shadow");
        }
      LOGGER_END_PROCESS (fav_process_name);

      this.theme_file_path = new Unity.ThemeFilePath ();
      this.icon_file_path = "";
      this.make_icon ();
    }

    construct
    {
    }

    /* hitting gconf too much is bad, so we want to make sure we only hit
     * when we are idle
     */
    public void do_save_priority ()
    {
      if (!this.queued_save_priority)
        {
          this.queued_save_priority = true;
          Idle.add (this.save_priority);
        }
    }

    public bool save_priority ()
    {
      this.queued_save_priority = false;
      if (!this.is_sticky) return false;
      var favorites = Launcher.Favorites.get_default ();
      string uid = get_fav_uid ();
      favorites.set_float (uid, "priority", this.priority);
      return false;
    }
    private void grab_priority ()
    {
      if (!this.is_sticky)
        {
          // we need something outside of this model to decide our priority
          this._priority = 1000000.0f;
          return;
        }
      // grab the current priority from gconf
      var favorites = Launcher.Favorites.get_default ();
      string uid = get_fav_uid ();
      float priority = favorites.get_float (uid, "priority");
      // because liblauncher has no error handling, we assume that a priority
      // of < 1.0f is unset. so generate a random new one and set
      if (priority < 1.0f)
        {
          priority = (float)Random.double_range (100000.0, 100002.0);
          favorites.set_float (uid, "priority", priority);
        }
      this._priority = priority;
    }

    private void on_app_icon_changed ()
    {
      make_icon ();
    }

    private void on_app_running_changed ()
    {
      notify_active ();
    }

    private void on_app_focus_changed ()
    {
      if (app.focused) {
        this.request_attention ();
      }
      notify_focused ();
    }

    private void on_app_urgent_changed ()
    {
      this.urgent_changed ();
    }

    private void on_app_opened (Wnck.Window window)
    {
      windows_changed ();
    }

    private void on_app_closed (Wnck.Window window)
    {
      windows_changed ();
    }

    public bool is_active
    {
      get { return this.app.running; }
    }

    public bool is_focused
    {
      get { return this.app.focused; }
    }

    public bool is_urgent
    {
      get { return this.app.get_urgent (); }
    }

    public Gdk.Pixbuf icon
    {
      get { return _icon; }
    }

    public string name
    {
      get { return this.app.name; }
    }

    public bool is_fixed {
      get {
        var favorites = Launcher.Favorites.get_default ();
        string uid = get_fav_uid ();
        if (uid == "") { return false; }
        return favorites.get_bool (uid, "fixed");
      }
    }

    public bool readonly {
      get {
        var favorites = Launcher.Favorites.get_default ();
        string uid = get_fav_uid ();
        if (uid == "") { return false; }
        return favorites.get_readonly (uid, "desktop_file");
      }
    }

    private bool _is_sticky;
    public bool is_sticky
    {
      get { return _is_sticky; }
      set
        {
          var favorites = Launcher.Favorites.get_default ();
          string uid = get_fav_uid ();
          if (uid != "" && !value)
            {
              // we are a favorite and we need to be unfavorited
              if (!favorites.get_readonly (uid, "desktop_file"))
              {
                favorites.remove_favorite (uid);
              }
            }
          else if (uid == "" && value)
            {
              string desktop_path = app.get_desktop_file ();
              uid = "app-" + Path.get_basename (desktop_path);
              // we are not a favorite and we need to be favorited!
              favorites.set_string (uid, "type", "application");
              favorites.set_string (uid, "desktop_file", desktop_path);
              favorites.add_favorite (uid);
            }
          _is_sticky = value;
          this.notify_active ();
        }
    }

    public Gee.ArrayList<ShortcutItem> get_menu_shortcuts ()
    {
      // get the desktop file
      Gee.ArrayList<ShortcutItem> ret_list = new Gee.ArrayList<ShortcutItem> ();
      var desktop_file = new KeyFile ();
      try
        {
          desktop_file.load_from_file (app.get_desktop_file (), 0);
        }
      catch (Error e)
        {
          warning ("Unable to load desktop file '%s': %s",
                   app.get_desktop_file (),
                   e.message);
          return ret_list;
        }

      var groups = desktop_file.get_groups ();
      for (int a = 0; a < groups.length; a++)
      {
        if ("QuickList Entry" in groups[a])
          {
            string exec = "";
            string name = "";
            try
              {
                exec = desktop_file.get_value (groups[a], "Exec");
                name = desktop_file.get_locale_string  (groups[a], "Name", "");
              } catch (Error e)
              {
                warning (e.message);
                continue;
              }
              var shortcut = new ApplicationShortcut ();
              shortcut.exec = exec;
              shortcut.name = name;
              shortcut.desktop_location = app.get_desktop_file ();
              ret_list.add (shortcut);
          }
      }
      return ret_list;
    }

    public Gee.ArrayList<ShortcutItem> get_menu_shortcut_actions ()
    {
      Gee.ArrayList<ShortcutItem> ret_list = new Gee.ArrayList<ShortcutItem> ();
      if (!this.readonly && !this.is_fixed)
        {
          var pin_entry = new LauncherPinningShortcut (this);
          ret_list.add (pin_entry);
        }

      if (this.app.running) {
        var open_entry = new LibLauncherShortcut ();
        open_entry.app = this.app;
        ret_list.add (open_entry);
      }

      return ret_list;
    }

    public bool ensure_state ()
    {
      this.on_app_focus_changed ();
      return false;
    }

    public void activate ()
    {
      if (app.running)
        {
          if (!this.app.focused)
            {
              // sigh, so hacky. i think clutters mainloop is blocking a signal
              // so lets just force a check of our focused status here
              Idle.add (this.ensure_state);
              app.show (Clutter.get_current_event_time ());
            }
        }
      else
        {
          try
            {
              app.launch ();
              this.activated ();
            }
          catch (GLib.Error e)
            {
              critical ("could not launch application %s: %s",
                        this.app.name,
                        e.message);
            }
        }
    }

    public void close ()
    {
      this.app.close (Clutter.get_current_event_time ());
    }

    /**
     * gets the favorite uid for this desktop file
     */
    private string fav_uid_cache = "";
    private string get_fav_uid ()
    {
      if (this.fav_uid_cache != "")
        return this.fav_uid_cache;

      string myuid = "";
      string my_desktop_path = app.get_desktop_file ();
      var favorites = Launcher.Favorites.get_default ();
      unowned SList<string> favorite_list = favorites.get_favorites();
      foreach (weak string uid in favorite_list)
        {
          // we only want favorite *applications* for the moment
          var type = favorites.get_string(uid, "type");
          if (type != "application")
              continue;

          string desktop_file = favorites.get_string(uid, "desktop_file");
          if (desktop_file == my_desktop_path)
            {
              myuid = uid;
            }
        }
      this.fav_uid_cache = myuid;
      return myuid;
    }

    public void regenerate_icon ()
    {
      string dfile = this.app.get_desktop_file ();
      this.app.set_desktop_file (dfile, true);
      make_icon ();
    }

    private void make_icon_from_wnck ()
    {
      unowned SList<Wnck.Window> windows = this.app.get_windows ();
      foreach (weak Wnck.Window window in windows)
        {
          Gdk.Pixbuf pixbuf = window.get_mini_icon ();
          if (pixbuf is Gdk.Pixbuf)
            {
              this._icon = pixbuf;
              break;
            }
        }
    }

    private void make_icon ()
    {
      string icon_name = this.app.icon_name;

      if (icon_name == null)
        {
          this.make_icon_from_wnck ();
          this.notify_icon ();
          return;
        }

      // first try to load from a path;
      if (this.try_load_from_file (icon_name))
        {
          this.notify_icon ();
          return;
        }

      //try to load from a path that we augment
      if (this.try_load_from_file ("/usr/share/pixmaps/" + icon_name))
        {
          this.notify_icon ();
          return;
        }

      this.theme_file_path = new Unity.ThemeFilePath ();

      // add our searchable themes
      Gtk.IconTheme theme = Gtk.IconTheme.get_default ();
      this.theme_file_path.add_icon_theme (theme);
      theme = new Gtk.IconTheme ();
      theme.set_custom_theme ("unity-icon-theme");
      this.theme_file_path.add_icon_theme (theme);
      theme.set_custom_theme ("Web");
      this.theme_file_path.add_icon_theme (theme);

      this.theme_file_path.found_icon_path.connect ((theme, filepath) => {
        try
          {
            this._icon = new Gdk.Pixbuf.from_file (filepath);
          }
        catch (Error e)
          {
            warning (@"Could not load from $filepath");
          }
        if (this._icon is Gdk.Pixbuf)
          {
            this.notify_icon ();
          }
      });
      this.theme_file_path.failed.connect (() => {
        // we didn't get an icon, so just load the failcon
        try
          {
            var default_theme = Gtk.IconTheme.get_default ();
            this._icon = default_theme.load_icon(Gtk.STOCK_MISSING_IMAGE, 48, 0);
          }
        catch (Error e)
          {
            warning (@"Could not load any icon for %s", this.app.name);
          }
          this.notify_icon ();
      });

      this.theme_file_path.get_icon_filepath (icon_name);
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
              this._icon = pixbuf;
              return true;
            }
        }
      return false;
    }
  }
}

