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
  
  public class ApplicationView : Ctk.Bin
  {
    
    private Launcher.Application app;
    private Ctk.Image icon;
    private int num_apps;
    private bool _is_sticky;
    
    /* if we are not sticky anymore and we are not running, request remove */
    public bool is_sticky {
      get { return _is_sticky; }
      set {
        if (value == false && !is_running) 
          this.request_remove ();
        _is_sticky = value;
      }
    }
    
    public bool is_running {
      get { return app.running; }
    }
    
    /**
     * signal is called when the application is not marked as sticky and 
     * it is not running
     */
    public signal void request_remove ();
    
    public ApplicationView (Launcher.Application app)
    {
      /* This is a 'view' for a launcher application object
       * it will (hopefully) be able to launch, track when this application 
       * opens and closes and be able to get desktop file info
       */
      num_apps = 0;
      this.app = app;
      this.icon = new Ctk.Image (42);
      add_actor(this.icon);
      generate_view_from_app ();
      
      this.app.opened.connect(this.on_app_opened);
      this.app.closed.connect(this.on_app_closed);
      
      button_press_event.connect(this.on_pressed);
      
      this.app.notify["running"].connect(this.notify_on_is_running);
      
      set_reactive(true);
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
      debug("%s", app.name);
      var pixbuf = make_icon (app.icon_name);
      this.icon.set_from_pixbuf (pixbuf);
    }
 
    private void on_app_opened (Wnck.Application wnck_app) 
    {
      debug("app %s opened", app.name);
    }

    private void on_app_closed (Wnck.Application wnck_app) 
    {
      debug("app %s closed", app.name);
    }
    
    private bool on_pressed(Clutter.Event src) 
    {
      app.launch ();
      return true;
    }
    
    /**
     * if the application is not running anymore and we are not sticky, we
     * request to be removed
     */
    private void notify_on_is_running ()
    {
      if (!this.is_running && !this.is_sticky)
          this.request_remove ();      
    }
          

  }
}
