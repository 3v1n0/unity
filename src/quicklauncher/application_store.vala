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

/*
 * Notes!
 * 
 * this application store object exists to half-manage the ApplicationView objects
 * this does not mean that it handles *everything* about them. for example the 
 * applicationview objects contain the LauncherApplication data provider so 
 * only they know when they truely need to be removed, if ever. (for example, 
 * sticky applications will never remove themselfs).
 * 
 * however we need a mechanism of loading new applications that are not sticky
 * and loading default favorites
 */

namespace Unity.Quicklauncher
{
  class ApplicationStore : Ctk.Bin
  {
    
    private Ctk.VBox container;
    private Gee.ArrayList<ApplicationView> widgets;
    private Gee.ArrayList<Launcher.Application> apps;
    
    private Launcher.Appman appman;
    private Launcher.Session session;
    
    public ApplicationStore ()
    {
    
      this.appman = Launcher.Appman.get_default ();
      this.session = Launcher.Session.get_default ();
      
      this.widgets = new Gee.ArrayList<ApplicationView> ();
      this.apps = new Gee.ArrayList<Launcher.Application> ();
      
      container = new Ctk.VBox (2);
      add_actor (container);
      
      build_favorites ();
      
      this.session.application_opened.connect (handle_session_application);
      
    }
    
    /**
     * goes though our favorites and addds them to the container
     * marks them as sticky also
     */
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
          
          var newapp = appman.get_application_for_desktop_file (desktop_file);
          add_application (newapp);
        }
    }
    
    private void handle_session_application (Launcher.Application app) 
    {
      /* before passing applications to add_application, we want to handle each
       * one to make sure its not a panel or whatever
       */
      /* this is *alkward*, have to go though the wnckapplications and then the 
       * wnckapplication windows and make sure that we have a valid window
       * i'll transfer this type of code to liblauncher soonish 
       * something like Launcher.Application.is_visible
       */
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
          add_application(app);
        }
        
    }
    
    /**
     * adds the Launcher.Application @app to this container
     */
    private void add_application (Launcher.Application app) 
    {
      if (app in this.apps)
        /* the application object will signal itself, we don't need to 
         * do a thing */
        return;
        
      var app_view = new ApplicationView (app);
      apps.add (app);
      widgets.add (app_view);
      container.pack(app_view, false, false);
    }
    
  }
}
