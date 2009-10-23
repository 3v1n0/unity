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
    private Gee.ArrayList<ApplicationView> apps;
    
    private Launcher.Appman appman;
    private Launcher.Session session;
    
    public ApplicationStore ()
    {
    
      this.appman = Launcher.Appman.get_default ();
      this.session = Launcher.Session.get_default ();
      
      this.apps = new Gee.ArrayList<ApplicationView> ();
    
      container = new Ctk.VBox (2);
      add_actor (container);
      
      /* for now just add some hard-coded applications here */
      var firefox = this.appman.get_application_for_desktop_file("/usr/share/applications/firefox-3.0.desktop");
      var evolution = this.appman.get_application_for_desktop_file("/usr/share/applications/evolution.desktop");
      var empathy = this.appman.get_application_for_desktop_file("/usr/share/applications/empathy.desktop");
      
      var tmpactor = new ApplicationView (firefox);
      apps.add (tmpactor);
      container.pack(tmpactor, false, false);
      
      tmpactor = new ApplicationView (evolution);
      apps.add (tmpactor);
      container.pack(tmpactor, false, false);
      
      tmpactor = new ApplicationView (empathy);
      apps.add (tmpactor);
      container.pack(tmpactor, false, false);
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
      apps.add (app_view);
      container.pack(tmpactor, false, false);
    }
    
  }
}
