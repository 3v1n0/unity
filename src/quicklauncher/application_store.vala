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
    
    
    
    
  }
}
