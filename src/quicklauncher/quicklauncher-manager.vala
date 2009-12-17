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
using Unity.Quicklauncher.Stores;
namespace Unity.Quicklauncher
{
 
  public class Manager : Ctk.Bin 
  {
    private Unity.Widgets.Scroller container;
    private Gee.HashMap<string,ApplicationStore> desktop_file_map;
    private Gee.HashMap<LauncherStore, LauncherView> store_map;

    private Launcher.Appman appman;
    private Launcher.Session session;

    construct 
    {
      logger_start_process ("/Unity/Quicklauncher");
      this.appman = Launcher.Appman.get_default ();
      this.session = Launcher.Session.get_default ();
      
      this.desktop_file_map = new Gee.HashMap<string, ApplicationStore> ();
      this.store_map = new Gee.HashMap<LauncherStore, LauncherView> ();

      this.container = new Unity.Widgets.Scroller (Ctk.Orientation.VERTICAL,
                                                   6);
      add_actor (container);

      build_favorites ();
      this.session.application_opened.connect (handle_session_application);
      
      Ctk.drag_dest_start (this.container);
      this.container.drag_motion.connect (on_drag_motion);
      this.container.drag_drop.connect (on_drag_drop);
      this.container.drag_data_received.connect (on_drag_data_received);
      
      logger_end_process ("/Unity/Quicklauncher");
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
        Gdk.Atom target_type = (Gdk.Atom) context.targets.nth_data (Unity.dnd_targets.TARGET_URL);
        if (target_type.name () == "")
        {
          warning ("bad DND type");
          return false;
        }
        Ctk.drag_get_data (actor, context, target_type, time_);
        target_type = (Gdk.Atom) context.targets.nth_data (Unity.dnd_targets.TARGET_STRING);
        if (target_type.name () == "")
        {
          warning ("bad DND type");
          return false;
        }
        Ctk.drag_get_data (actor, context, target_type, time_);
        debug ("asking for data");
      } else 
      {
        warning ("got a strange dnd");
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
      debug ("got dnd data");
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
        warning ("dnd transfer failed");
      }
      Gtk.drag_finish (context, dnd_success, delete_selection_data, time_);
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
      if ("http" in split_uri[0])
      {
        //we have a http url, prism it
        var webapp = new Prism (clean_uri);
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
      
      build_favorites ();
      return true;
    }

    private void build_favorites () 
    {
      string Domain = "/Unity/Quicklauncher/Favorites/";
      logger_start_process (Domain);
      var favorites = Launcher.Favorites.get_default ();
      
      unowned SList<string> favorite_list = favorites.get_favorites();
      foreach (weak string uid in favorite_list)
        {
          logger_start_process (Domain + uid);
          // we only want favorite *applications* for the moment 
          var type = favorites.get_string(uid, "type");
          if (type != "application")
              continue;
              
          string desktop_file = favorites.get_string(uid, "desktop_file");
          assert (desktop_file != "");
          
          if (!(desktop_file in desktop_file_map.keys))
            {
              ApplicationStore store = get_store_for_desktop_file (desktop_file);
              store.is_sticky = true;
              LauncherView view = get_view_for_store (store);
          
              add_view (view);
            }
          
          logger_end_process (Domain + uid);
        }
      
      logger_end_process (Domain);
    }


   private void handle_session_application (Launcher.Application app) 
    { 
      

      bool app_is_visible = false;
      
      unowned GLib.SList<Wnck.Application> wnckapps = app.get_wnckapps ();
      foreach (Wnck.Application wnckapp in wnckapps)
        {
          unowned GLib.List<Wnck.Window> windows = wnckapp.get_windows ();
          foreach (Wnck.Window window in windows)
            {
              var type = window.get_window_type ();
              if (!(type == Wnck.WindowType.DESKTOP
                    || type == Wnck.WindowType.DOCK
                    || type == Wnck.WindowType.SPLASHSCREEN
                    || type == Wnck.WindowType.MENU))
              {
                app_is_visible = true;
              }
            }
        }
        
      if (app_is_visible)
        {
          var desktop_file = app.get_desktop_file ();
          if (desktop_file != null) 
          {
            ApplicationStore store = get_store_for_desktop_file (desktop_file);
            LauncherView view = get_view_for_store (store);
            if (view.get_parent () == null)
            {
              add_view (view);
            }
          }
        }
    }
    

    private ApplicationStore get_store_for_desktop_file (string uri)
    {
      /* we check to see if we already have this desktop file loaded, 
       * if so, we just use that one instead of creating a new store
       */
      if (uri in desktop_file_map.keys)
        {
          return desktop_file_map[uri];
        }

      ApplicationStore store = new ApplicationStore (uri);
      desktop_file_map[uri] = store;
      return store;
    }
    
    private LauncherView get_view_for_store (LauncherStore store)
    {
      if (store in store_map.keys)
        {
          return store_map[store];
        }
        
      LauncherView view = new LauncherView (store);
      store_map[store] = view;
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
      container.add_actor(view);
      view.request_remove.connect(remove_view);
    }
    
    private void remove_view (LauncherView view)
    {
      // for now just remove the application quickly. at some point
      // i would assume we have to pretty fading though, thats trivial to do
      
      this.container.remove_actor (view);
    } 

  }
}
