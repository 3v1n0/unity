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

namespace Unity.Quicklauncher.Stores
{
  
  
  public class ApplicationStore : GLib.Object, LauncherStore 
  {
    private Gdk.Pixbuf _icon;
    private Launcher.Application app;
    private Launcher.Appman manager;
    
    public ApplicationStore (string desktop_uri)
    {
      this.manager = Launcher.Appman.get_default ();
      this.app = this.manager.get_application_for_desktop_file (desktop_uri);
      this.app.opened.connect(this.on_app_opened);

      this.app.notify["focused"].connect (this.notify_on_focused);
      this.app.notify["running"].connect (this.notify_on_running);
      
      this._icon = make_icon (app.icon_name);
    }
    
    construct
    {
    }
    
    private void on_app_opened (Wnck.Application app) 
    {
      this.request_attention ();
    }
    
    private void notify_on_focused ()
    {
      if (app.focused) {
        this.request_attention ();
      }
      notify_focused();
    }

    private void notify_on_running () 
    {
      notify_active ();     
    }

    public bool is_active 
    {
      get { return this.app.running; }
    }
    public bool is_focused 
    {
      get { return this.app.focused; }
    }
    public Gdk.Pixbuf icon 
    {
      get { return _icon; }
    }
    public string name 
    {
      get { return this.app.name; }
    }
    
    private bool _is_sticky;
    public bool is_sticky 
    {
      get { return _is_sticky; }
      set 
        { 
          var favorites = Launcher.Favorites.get_default ();
          string uid = get_fav_uid ();
          if (uid != "" && !value) 
            {
              // we are a favorite and we need to be unfavorited
              favorites.remove_favorite (uid);
            }
          else if (uid == "" && value)
            {
              string desktop_path = app.get_desktop_file ();
              uid = "app-" + Path.get_basename (desktop_path);
              // we are not a favorite and we need to be favorited!
              favorites.set_string (uid, "type", "application");
              favorites.set_string (uid, "desktop_file", desktop_path);
              favorites.add_favorite (uid);
            }
          _is_sticky = value;
        }
    }

    public void activate ()
    {
      if (app.running)
        {
          // we only want to switch to the application, not launch it
          app.show ();
        }
      else 
        {
          try
            {
              app.launch ();
            } 
          catch (GLib.Error e)
            {
              critical ("could not launch application %s: %s", 
                        this.app.name, 
                        e.message);
            }
        }
    }
    
    /**
     * gets the favorite uid for this desktop file
     */
    private string get_fav_uid ()
    {
      string myuid = "";
      string my_desktop_path = app.get_desktop_file ();
      var favorites = Launcher.Favorites.get_default ();
      unowned SList<string> favorite_list = favorites.get_favorites();
      foreach (weak string uid in favorite_list)
        {
          // we only want favorite *applications* for the moment 
          var type = favorites.get_string(uid, "type");
          if (type != "application")
              continue;
              
          string desktop_file = favorites.get_string(uid, "desktop_file");
          if (desktop_file == my_desktop_path)
            {
              myuid = uid;
            }
        }
      return myuid;
    }
  
    /** 
     * taken from the prototype code and shamelessly stolen from
     * netbook launcher. needs to be improved at some point to deal
     * with all cases, it will miss some apps at the moment
     */
    static Gdk.Pixbuf make_icon(string? icon_name) 
    {
      /*
       * This code somehow manages to miss a lot of icon names 
       * (non found icons are replaced with stock missing image icons)
       * which is a little strange as I ported this code fron netbook launcher
       * pixbuf-cache.c i think, If anyone else has a better idea for this then
       * please give it a go. otherwise i will revisit this code the last week 
       * of the month sprint
       */
      Gdk.Pixbuf pixbuf = null;
      Gtk.IconTheme theme = Gtk.IconTheme.get_default ();
      
      if (icon_name == null)
        {
          try 
            {
              pixbuf = theme.load_icon(Gtk.STOCK_MISSING_IMAGE, 42, 0);
            }
          catch (Error e)
            {
              warning ("Unable to load stock image: %s", e.message);
              pixbuf = null;
            }

          return pixbuf;
        }
        
      if (icon_name.has_prefix("file://")) 
        {
          string filename = "";
          /* this try/catch sort of isn't needed... but it makes valac stop 
           * printing warning messages
           */
          try 
          {
            filename = Filename.from_uri(icon_name);
          } 
          catch (GLib.ConvertError e)
          {
          }
          if (filename != "") 
            {
              try
                {
                  pixbuf = new Gdk.Pixbuf.from_file_at_scale(icon_name, 
                                                             42, 42, true);
                }
              catch (Error e)
                {
                  warning ("Unable to load pixbuf from file '%s': %s",
                           icon_name,
                           e.message);
                }
              if (pixbuf is Gdk.Pixbuf)
                  return pixbuf;
            }
        }
      
      if (Path.is_absolute(icon_name))
        {
          if (FileUtils.test(icon_name, FileTest.EXISTS)) 
            {
              try
                {
                  pixbuf = new Gdk.Pixbuf.from_file_at_scale(icon_name, 
                                                             42, 42, true);
                }
              catch (Error e)
                {
                  warning ("Unable to load image from file '%s': %s",
                           icon_name,
                           e.message);
                }

              if (pixbuf is Gdk.Pixbuf)
                return pixbuf;
            }
        }

      if (FileUtils.test ("/usr/share/pixmaps/" + icon_name, 
                          FileTest.IS_REGULAR))
        {
          try
            {
              pixbuf = new Gdk.Pixbuf.from_file_at_scale (
                    "/usr/share/pixmaps/" + icon_name, 42, 42, true);
            }
          catch (Error e)
            {
              warning ("Unable to load image from file '%s': %s",
                       "/usr/share/pixmaps/" + icon_name,
                       e.message);
            }
          
          if (pixbuf is Gdk.Pixbuf)
            return pixbuf;
        }
      
      Gtk.IconInfo info = theme.lookup_icon(icon_name, 42, 0);
      if (info != null) 
        {
          string filename = info.get_filename();
          if (FileUtils.test(filename, FileTest.EXISTS)) 
            {
              try
                {
                  pixbuf = new Gdk.Pixbuf.from_file_at_scale(filename, 
                                                             42, 42, true);
                }
              catch (Error e)
                {
                  warning ("Unable to load image from file '%s': %s",
                           filename,
                           e.message);
                }
            
              if (pixbuf is Gdk.Pixbuf)
                return pixbuf;
            }
        }
      
      try 
      {
        pixbuf = theme.load_icon(icon_name, 42, Gtk.IconLookupFlags.FORCE_SVG);
      }
      catch (GLib.Error e) 
      {
        warning ("could not load icon for %s - %s", icon_name, e.message);
        try
          {
            pixbuf = theme.load_icon(Gtk.STOCK_MISSING_IMAGE, 42, 0);
          }
        catch (Error err)
          {
            warning ("Unable to load icon for %s: %s", icon_name, err.message);
          }
        return pixbuf;
      }
      return pixbuf;
          
    }
  }
}

