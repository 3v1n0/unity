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
using GtkClutter;

namespace Unity {
  
  public interface UnderlayWindow : Gtk.Window
  {
    public abstract void setup_window ();
    public abstract void set_fullscreen ();
  }
  
  public class Workarea 
  {
    public signal void workarea_changed ();
    public int left;
    public int top;
    public int right;
    public int bottom;
    
    public Workarea ()
    {
      left = 0;
      right = 0;
      top = 0;
      bottom = 0;
      
      update_net_workarea ();
    }
    

    public void update_net_workarea () 
    {
      /* FIXME - steal code from the old liblauncher to do this (launcher-session.c)
       * this is just fake code to get it running
       */
      
      left = 0;
      right = 800;
      top = 0;
      bottom = 600;
    }
  }
  
  public class Window : Gtk.Window, UnderlayWindow
  {
    
    private GtkClutter.Embed gtk_clutter;
    public Clutter.Stage stage;
    private Workarea workarea_size;
    
    public void setup_window ()
    {
      this.decorated = false;
      this.skip_taskbar_hint = true;
      this.skip_pager_hint = true;
      this.type_hint = Gdk.WindowTypeHint.NORMAL;
      
      this.realize ();
    }
    
    public void set_fullscreen ()
    {
      int width;
      int height;
      
      width = this.workarea_size.right - this.workarea_size.left;
      height = this.workarea_size.bottom - this.workarea_size.top;

      resize (width, height);
      this.stage.set_size(width, height);
      
    }
    
    public Window ()
    {
      this.workarea_size = new Workarea ();
      this.workarea_size.update_net_workarea ();
      
      setup_window ();
      
      this.gtk_clutter = new GtkClutter.Embed ();
      this.add (this.gtk_clutter);
      this.gtk_clutter.realize ();

      this.stage = (Clutter.Stage)this.gtk_clutter.get_stage ();
      Clutter.Color stage_bg = Clutter.Color ()
        { 
          red = 0x00,
          green = 0x00,
          blue = 0x00,
          alpha = 0xB0
        };
      this.stage.set_color (stage_bg);
      this.show_all ();
      this.stage.show_all ();
    }
    
  }
}
