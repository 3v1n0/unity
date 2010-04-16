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
using Unity.Quicklauncher.Models;
using Unity.Webapp;

namespace Unity.Quicklauncher
{

  public class Manager : Ctk.Bin
  {
    private Unity.Widgets.Scroller container;
    private Gee.ArrayList<Launcher.Application> launcher_apps;
    private Gee.HashMap<LauncherModel, LauncherView> model_map;

    private Launcher.Appman appman;
    private Launcher.Session session;
    private string webapp_device;

    private Unity.Webapp.WebiconFetcher webicon_fetcher;

    // lazy init of gconf here :-) wait's until we need it
    private GConf.Client? gconf_client_;
    private GConf.Client? gconf_client {
      get {
        if (this.gconf_client_ is GConf.Client)
          {
            return this.gconf_client_;
          }
        else
          {
            return this.gconf_client_ = GConf.Client.get_default ();
          }
      }
    }

    private LauncherView _active_launcher;
    public LauncherView active_launcher {
      get { return _active_launcher; }
      private set {
        LauncherView prev = _active_launcher;
        _active_launcher = value;
        active_launcher_changed (prev, _active_launcher);
      }
    }

    public signal void active_launcher_changed (LauncherView last, LauncherView current);

    construct
    {
      START_FUNCTION ();

      Unity.Testing.ObjectRegistry.get_default ().register ("QuicklauncherManager",
                                                            this);

      this.appman = Launcher.Appman.get_default ();
      this.appman.get_default ().enable_window_checking = true;
      this.session = Launcher.Session.get_default ();

      launcher_apps = new Gee.ArrayList<Launcher.Application> ();
      this.model_map = new Gee.HashMap<LauncherModel, LauncherView> ();

      this.container = new Unity.Widgets.Scroller (Ctk.Orientation.VERTICAL,
                                                   0);
      add_actor (container);
      build_favorites ();
      this.session.application_opened.connect (handle_session_application);


      Ctk.drag_dest_start (this.container);
      this.container.drag_motion.connect (on_drag_motion);
      this.container.drag_drop.connect (on_drag_drop);
      this.container.drag_data_received.connect (on_drag_data_received);

      //watch the global shell for new icon cache changes
      Unity.global_shell.need_new_icon_cache.connect (this.on_webicon_built);

      // put a watch on our gconf key so we get notified of new favorites
      try {
          gconf_client.add_dir ("/desktop/unity/launcher/favorites",
                                GConf.ClientPreloadType.NONE);
          gconf_client.notify_add ("/desktop/unity/launcher/favorites/favorites_list", this.on_favorite_change);
      } catch (Error e) {
      }
      this.webapp_device = get_webapp_device ();

      END_FUNCTION ();
    }

    public bool refresh_models ()
    {
      foreach (LauncherModel model in this.model_map.keys)
        {
          model.refresh ();
        }
      return false;
    }

    private void on_favorite_change ()
    {
      this.build_favorites ();
    }

    // returns which string is the currently set webapp device
    private string get_webapp_device ()
    {
      //check to see the unity dir exists
      try {
        bool dir_exists = this.gconf_client.dir_exists (UNITY_CONF_PATH);
        if (!dir_exists)
          {
            // make the directory
            this.gconf_client.add_dir (UNITY_CONF_PATH,
                                       GConf.ClientPreloadType.NONE);
          }
      }
      catch (Error e) {
        warning ("%s", e.message);
      }

      try {
        bool value = this.gconf_client.get_bool (UNITY_CONF_PATH + "/launcher/webapp_use_chromium");
        if (value)
          {
            return "chromium";
          }
      }
      catch (Error e) {
        return "prism";
      }
      return "prism";
    }

    private bool on_drag_motion (Ctk.Actor actor, Gdk.DragContext context,
                                 int x, int y, uint time_)
    {
      return true;
    }

    private bool on_drag_drop (Ctk.Actor actor, Gdk.DragContext context,
                               int x, int y, uint time_)
    {
      if (context.targets != null)
      {
        Gtk.TargetList tl;
        Gdk.Atom target_type;
        // silly tricking vala into passing an array with no elements.
        // if anyone knows a way of doing this properly, please replace!
        Gtk.TargetEntry[]? targets = null;
        tl = new Gtk.TargetList (targets);
        tl.add_uri_targets (0);

        target_type = Ctk.drag_dest_find_target (context, tl);

        if (target_type.name () == "")
        {
          return false;
        }
        Ctk.drag_get_data (actor, context, target_type, time_);
        unowned GLib.List<Gdk.Atom?> list = context.targets;

        target_type = (Gdk.Atom) list.nth_data (Unity.dnd_targets.TARGET_STRING);
        if (target_type.name () == "")
        {
          return false;
        }
        Ctk.drag_get_data (actor, context, target_type, time_);
      } else
      {
        return false;
      }
      return true;
    }

    private void on_drag_data_received (Ctk.Actor actor,
                                        Gdk.DragContext context, int x, int y,
                                        Gtk.SelectionData data, uint target_type,
                                        uint time_)
    {
      bool dnd_success = false;
      bool delete_selection_data = false;
      // Deal with what we are given from source
      if ((data != null) && (data.length >= 0)) {
        if (context.action == Gdk.DragAction.MOVE) {
          delete_selection_data = true;
        }

        switch (target_type) {
        case Unity.dnd_targets.TARGET_URL:
          // we got a uri, forward it to the uri handler
          dnd_success = handle_uri ((string) data.data);
          break;
        default:
          break;
        }
      }

      if (dnd_success == false) {
      }
      Gtk.drag_finish (context, dnd_success, delete_selection_data, time_);
    }

    private bool test_url (string url)
      {
        // just returns if the given string is really a url
        try {
          Regex match_url = new Regex (
            """((https?|s?ftp):((//)|(\\\\))+[\w\d:#@%/;$()~_?\+-=\\\.&]*)"""
          );
          if (match_url.match (url))
            {
              return true;
            }

        } catch (RegexError e) {
          warning ("%s", e.message);
          return false;
        }
        return false;
      }

    private bool handle_uri (string uri)
    {
      string clean_uri = uri.split("\n", 2)[0].split("\r", 2)[0];
      try {
        var regex = new Regex ("\\s");
        clean_uri = regex.replace (clean_uri, -1, 0, "");
      } catch (RegexError e) {
        warning ("%s", e.message);
      }
      clean_uri.strip();
      var split_uri = clean_uri.split ("://", 2);
      if (test_url (clean_uri))
      {
        //we have a http url, webapp it
        var icon_dirstring = Environment.get_home_dir () + "/.local/share/icons/";
        var icon_directory = File.new_for_path (icon_dirstring);
        try {
          if (!icon_directory.query_exists (null))
            {
              icon_directory.make_directory_with_parents (null);
            }
        } catch (Error e) {
          // do nothing, errors are fine :)
        }
        var split_url = clean_uri.split ("://", 2);
        var name = split_url[1];
        var hostname = Unity.Webapp.get_hostname (clean_uri);
        // gotta sanitze our name
        try {
          var regex = new Regex ("(/)");
          name = regex.replace (name, -1, 0, "-");
        } catch (RegexError e) {
          warning ("%s", e.message);
        }

          var webapp = new ChromiumWebApp (clean_uri, hostname + ".png");
          this.webicon_fetcher = new Unity.Webapp.WebiconFetcher (clean_uri,
                                                                  icon_dirstring + hostname + ".svg",
                                                                  webapp.desktop_file_path ());
          this.webicon_fetcher.fetch_webapp_data ();
          webapp.add_to_favorites ();
      }

      else if (".desktop" in Path.get_basename (clean_uri))
      {
        // we have a potential desktop file, test it.
        try
        {
          var desktop_file = new KeyFile ();
          desktop_file.load_from_file (split_uri[1], 0);
        } catch (Error e)
        {
          // not a desktop file
          error (e.message);
          return false;
        }

        var favorites = Launcher.Favorites.get_default ();
        string uid = "app-" + Path.get_basename (clean_uri);
        favorites.set_string (uid, "type", "application");
        favorites.set_string (uid, "desktop_file", split_uri[1]);
        favorites.add_favorite (uid);
      }
      else
      {
        return false;
      }

      //build_favorites ();
      return true;
    }

    private void on_webicon_built ()
    {
      foreach (LauncherModel model in this.model_map.keys)
      {
        model.regenerate_icon ();
      }
    }

    private void build_favorites ()
    {
      START_FUNCTION ();
      var favorites = Launcher.Favorites.get_default ();

      unowned SList<string> favorite_list = favorites.get_favorites();
      foreach (weak string uid in favorite_list)
        {
          string process_name = "favorite" + uid;
          LOGGER_START_PROCESS (process_name);
          // we only want favorite *applications* for the moment
          var type = favorites.get_string(uid, "type");
          if (type != "application")
              continue;

          string? desktop_file = favorites.get_string(uid, "desktop_file");
          assert (desktop_file != "");
          if (!FileUtils.test (desktop_file, FileTest.EXISTS))
            {
              // we don't have a desktop file that exists, remove it from the
              // favourites
              favorites.remove_favorite (uid);
            }
          else
            {
              string launcher_process_name = "Launcher-" + process_name;
              LOGGER_START_PROCESS (launcher_process_name);
              Launcher.Application application = appman.get_application_for_desktop_file (desktop_file);

              if (launcher_apps.contains (application))
                continue;
              launcher_apps.add (application);
              LOGGER_END_PROCESS (launcher_process_name);

              string model_process_name = "Model-" + process_name;
              LOGGER_START_PROCESS (model_process_name);
              ApplicationModel model = new ApplicationModel (application);
              model.is_sticky = true;
              LOGGER_END_PROCESS (model_process_name);
              string view_process_name = "View-" + process_name;
              LOGGER_START_PROCESS (view_process_name);
              LauncherView view = get_view_for_model (model);
              LOGGER_END_PROCESS (view_process_name);

              add_view (view);
            }

          LOGGER_END_PROCESS (process_name);
        }

      END_FUNCTION ();
    }

    private float get_last_priority ()
    {
      float max_priority = 0.0f;
      foreach (LauncherModel model in this.model_map.keys)
        {
          if (!(model is ApplicationModel))
            continue;
          max_priority = Math.fmaxf ((model as ApplicationModel).priority, max_priority);
        }
      return max_priority;
    }

    private void handle_session_application (Launcher.Application app)
    {
      if (launcher_apps.contains (app))
        return;
      launcher_apps.add (app);
      ApplicationModel model = new ApplicationModel (app);

      string desktop_file = app.get_desktop_file ();
      if (desktop_file != null && !model.is_sticky)
        {
          model.priority = get_last_priority ();
        }

      LauncherView view = get_view_for_model (model);
      if (view.get_parent () == null)
        {
          add_view (view);
        }
    }

    private LauncherView get_view_for_model (LauncherModel model)
    {
      if (model in this.model_map.keys)
        {
          return this.model_map[model];
        }

      LauncherView view = new LauncherView (model);
      this.model_map[model] = view;
      return view;
    }


    /**
     * adds the LauncherView @view to the launcher
     */
    private void add_view (LauncherView view)
    {
      if (view == null)
      {
        return;
      }
      if (view.model.is_fixed)
        {
          container.add_actor (view, true);
        }
      else
        {
          container.add_actor (view, false);
        }
      view.request_remove.connect(remove_view);
      view.enter_event.connect (on_launcher_enter_event);
      view.leave_event.connect (on_launcher_leave_event);
    }

    private void remove_view (LauncherView view)
    {
      // for now just remove the application quickly. at some point
      // i would assume we have to pretty fading though, thats trivial to do
      this.container.remove_actor (view);
      view.enter_event.disconnect (on_launcher_enter_event);
      view.leave_event.disconnect (on_launcher_leave_event);

      if (view.model is ApplicationModel)
        {
          launcher_apps.remove ((view.model as ApplicationModel).app);
        }
    }

    private bool on_launcher_enter_event (Clutter.Event event)
    {
      active_launcher = event.get_source () as LauncherView;
      return false;
    }

    private bool on_launcher_leave_event (Clutter.Event event)
    {
      if (active_launcher == event.get_source () as LauncherView)
        active_launcher = null;
      return false;
    }
  }
}
