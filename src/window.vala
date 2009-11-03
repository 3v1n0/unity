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
 *             Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */

namespace Unity
{

  public class UnderlayWindow : Gtk.Window, Shell
  {
    public bool is_popup { get; construct; }
    public int  popup_width { get; construct; }
    public int  popup_height { get; construct; }
    
    private Workarea workarea_size;

    private GtkClutter.Embed gtk_clutter;
    private Clutter.Stage    stage;

    private Background         background;
    private Quicklauncher.View quicklauncher;

    public UnderlayWindow (bool popup, int width, int height)
    {
      this.is_popup = popup;
      this.popup_width = width;
      this.popup_height = height;
    }
    
    construct
    {
      this.workarea_size = new Workarea ();
      this.workarea_size.update_net_workarea ();
      
      if (this.is_popup)
        {
          this.type_hint = Gdk.WindowTypeHint.NORMAL;
          this.decorated = true;
          this.skip_taskbar_hint = false;
          this.skip_pager_hint = false;
          this.delete_event.connect (() => 
            {
              Gtk.main_quit ();
              return false;
            }
          );
        }
      else
        {
          this.type_hint = Gdk.WindowTypeHint.DESKTOP;
          this.decorated = false;
          this.skip_taskbar_hint = true;
          this.skip_pager_hint = true;
          this.delete_event.connect (() => { return true; });
        }
      this.title = "Unity";
      
      /* Gtk.ClutterEmbed */
      this.realize ();

      this.gtk_clutter = new GtkClutter.Embed ();
      this.add (this.gtk_clutter);
      this.gtk_clutter.realize ();
      
      this.stage = (Clutter.Stage)this.gtk_clutter.get_stage ();
      Clutter.Color stage_bg = Clutter.Color () { 
          red = 0x00,
          green = 0x00,
          blue = 0x00,
          alpha = 0xff 
        };
      this.stage.set_color (stage_bg);

      /* Components */
      this.background = new Background ();
      this.stage.add_actor (this.background);
      this.background.show ();

      this.quicklauncher = new Quicklauncher.View (this);
      this.stage.add_actor (this.quicklauncher);
      
      /* Layout everything */
      this.resize_window ();
    }

    private void resize_window ()
    {
      int width, height;

      if (this.is_popup)
        {
          width = this.popup_width;
          height = this.popup_height;
        }
      else
        {
          Gdk.Rectangle size;

          this.screen.get_monitor_geometry (0, out size);
          width = size.width;
          height = size.height;
        }

      debug ("UnderlayWindow: resize: %dx%d", width, height);

      this.resize (width, height);
      this.stage.set_size (width, height);

      if (!this.is_popup)
        Utils.set_strut ((Gtk.Window)this, 54, 0, height);

      /* Update component layouts */
      this.background.set_position (0, 0);
      this.background.set_size (width, height);
      
      this.quicklauncher.set_size (54, height);
      this.quicklauncher.set_position (this.workarea_size.left,
                                       this.workarea_size.top);
    }

    public override void show ()
    {
      base.show ();
      this.gtk_clutter.show ();
      this.stage.show_all ();
    }

    /*
     * SHELL IMPLEMENTATION
     */
    public Clutter.Stage get_stage ()
    {
      return this.stage;
    }

    public ShellMode get_mode ()
    {
      return ShellMode.UNDERLAY;
    }
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
      /* FIXME - steal code from the old liblauncher to do this
       * (launcher-session.c) this is just fake code to get it running
       */
      left = 0;
      right = 0;
      top = 24;
      bottom = 64;
    }
  }  
}
