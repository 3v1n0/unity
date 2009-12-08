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
      this.appman = Launcher.Appman.get_default ();
      this.session = Launcher.Session.get_default ();
      
      this.desktop_file_map = new Gee.HashMap<string, ApplicationStore> ();
      this.store_map = new Gee.HashMap<LauncherStore, LauncherView> ();

      this.container = new Unity.Widgets.Scroller (Ctk.Orientation.VERTICAL,
                                                   6);
      add_actor (container);

      build_favorites ();
      this.session.application_opened.connect (handle_session_application);
    }

    private void build_favorites () 
    {
      var favorites = Launcher.Favorites.get_default ();
      
      unowned SList<string> favorite_list = favorites.get_favorites();
      foreach (weak string uid in favorite_list)
        {
          
          // we only want favorite *applications* for the moment 
          var type = favorites.get_string(uid, "type");
          if (type != "application")
              continue;
              
          string desktop_file = favorites.get_string(uid, "desktop_file");
          assert (desktop_file != "");
          
          ApplicationStore store = get_store_for_desktop_file (desktop_file);
          store.is_sticky = true;
          LauncherView view = get_view_for_store (store);
          
          add_view (view);
        }
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
