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
 * Authored by Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */

namespace Unity
{  
  
  public class QuickLauncher : Object
  {

    /* this is just a toplevel for now, until some sort of api between unity
     * and its interfaces is standardised
     */
    public Clutter.Actor _view;
    public Clutter.Actor view {
        get { return _view; }
        set { _view = value; }
    }
    
    private Launcher.Appman appman;
    private Launcher.Session session;
    
    public QuickLauncher (Clutter.Stage stage)
    { 
      this.appman = Launcher.Appman.get_default ();
      this.session = Launcher.Session.get_default ();
      view = Ctk.Toplevel.get_default_for_stage (stage);
      stage.add_actor (view);
      
      var myview = view as Ctk.Toplevel;
      var firefox = this.appman.get_application_for_desktop_file("/usr/share/applications/firefox-3.0.desktop");
      var tmpactor = new Unity.ApplicationView (firefox);
      
      myview.add_actor (tmpactor);
    }
  }
}
