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

namespace Unity.Quicklauncher
{  
  public class Main : Object
  {

    /* this is just a toplevel for now, until some sort of api between unity
     * and its interfaces is standardised
     */
    public Clutter.Actor _view;
    public Clutter.Actor view {
        get { return _view; }
        set { _view = value; }
    }
  
    private ApplicationStore store;
    
    public Main (Clutter.Stage stage)
    { 
      view = Ctk.Toplevel.get_default_for_stage (stage);
      stage.add_actor (view);
      
      var myview = view as Ctk.Toplevel;
      store = new ApplicationStore ();
      myview.add_actor (store);
	}
  }
}
