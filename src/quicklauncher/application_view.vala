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

namespace Unity
{  
  
  public class ApplicationView : Ctk.Bin
  {
    
    private Launcher.Application app;
    private Ctk.Image icon;
    
    public ApplicationView (Launcher.Application app)
    {
      /* This is a 'view' for a launcher application object
       * it will (hopefully) be able to launch, track when this application 
       * opens and closes and be able to get desktop file info
       */
      
      this.app = app;
      this.icon = new Ctk.Image (42);
      add_actor(this.icon);
      generate_view_from_app ();
      
      this.app.connect.opened(this.on_app_opened);
      this.app.connect.closed(this.on_app_closed);
    }
    
    /** 
     * taken from the prototype code and shamelessly stolen from
     * netbook launcher. needs to be improved at some point to deal
     * with all cases, it will miss some apps at the moment
     */
    protected static Gdk.Pixbuf make_icon(string icon_name) 
    {
      Gdk.Pixbuf pixbuf;
      Gtk.IconTheme theme = new Gtk.IconTheme();
      
      if (icon_name.has_prefix("file://")) 
        {
          string filename = Filename.from_uri(icon_name);
          if (filename != "") 
            {
              pixbuf = new Gdk.Pixbuf.from_file_at_scale(icon_name, 42, 42, true);
              if (pixbuf is Gdk.Pixbuf)
                  return pixbuf;
            }
        }
      
      if (Path.is_absolute(icon_name))
        {
          if (FileUtils.test(icon_name, FileTest.EXISTS)) 
            {
              pixbuf = new Gdk.Pixbuf.from_file_at_scale(icon_name, 42, 42, true);

              if (pixbuf is Gdk.Pixbuf)
                return pixbuf;
            }
        }
      
      Gtk.IconInfo info = theme.lookup_icon(icon_name, 42, 0);
      if (info != null) 
        {
          string filename = info.get_filename();
          pixbuf = new Gdk.Pixbuf.from_file_at_scale(filename, 42, 42, true);
          
          if (pixbuf is Gdk.Pixbuf)
            return pixbuf;
        }
      
      pixbuf = theme.load_icon(icon_name, 42, Gtk.IconLookupFlags.FORCE_SVG);
      
      return pixbuf;
          
    }
    
    /**
     * goes though our launcher application and generates icons and such
     */
    private void generate_view_from_app ()
    {
      var pixbuf = make_icon (app.name);
      this.icon.set_from_pixbuf (pixbuf);
    }
 
    private on_app_opened (void *wnckapp, void *p1) 
    {
      /* the launcherapi needs metadata fixes here first */
    }

    private on_app_closed (void *wnckapp, void *p1) 
    {
      /* the launcherapi needs metadata fixes here first */
    }

  }
}
