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
 * Authored by Jason Smith <jason.smith@canonical.com>
 *
 */
 
namespace Unity {
  
  public class SpacesManager : GLib.Object
  {
    List<Clutter.Actor> clones;
    Plugin plugin;
    unowned Mutter.MetaScreen screen;
    
    public uint top_padding { get; set; }
    public uint right_padding { get; set; }
    public uint bottom_padding { get; set; }
    public uint left_padding { get; set; }
    
    public uint spacing { get; set; }
    public bool showing { get; private set; }
    
    public SpacesManager (Plugin plugin) {
      this.plugin = plugin;
    }
    
    construct
    {
      SetPadding (50, 50, 50, 50);
      spacing = 15;
    }
    
    public void SetPadding (uint top, uint right, uint left, uint bottom) {
      top_padding    = top;
      right_padding  = right;
      left_padding   = left;
      bottom_padding = bottom;
    }
    
    public void ShowSpacesPicker () {
      showing = true;
      plugin.add_fullscreen_request (this);
      
      unowned Mutter.MetaScreen screen = plugin.plugin.get_screen (); 
      unowned GLib.List<Mutter.MetaWorkspace> workspaces = Mutter.MetaScreen.get_workspaces (screen);
      unowned Clutter.Container window_group = plugin.plugin.get_window_group() as Clutter.Container;
      
      foreach (unowned Mutter.MetaWorkspace workspace in workspaces)
        {
          Clutter.Actor clone = WorkspaceClone (workspace);
          clones.prepend (clone);
          
          window_group.add_actor (clone);
          clone.reactive = true;
          clone.raise_top ();
          clone.show ();
          
          unowned Mutter.MetaWorkspace cpy = workspace;
          clone.button_release_event.connect (() => { select_workspace (cpy); return true; });
        }
      
      LayoutWorkspaces (clones, screen);
      
      unowned GLib.List<Mutter.Window> windows = plugin.plugin.get_windows ();
      foreach (Mutter.Window w in windows)
        {
            (w as Clutter.Actor).opacity = 0;
        }
    }
    
    private void select_workspace (Mutter.MetaWorkspace workspace) {
      foreach (Clutter.Actor clone in clones)
        {
          clone.destroy ();
        }
      
      unowned GLib.List<Mutter.Window> windows = plugin.plugin.get_windows ();
      foreach (Mutter.Window w in windows)
        {
            (w as Clutter.Actor).opacity = 255;
        }
      
      clones = null;
      
      uint time_ = Mutter.MetaDisplay.get_current_time (Mutter.MetaScreen.get_display (Mutter.MetaWorkspace.get_screen (workspace)));
      Mutter.MetaWorkspace.activate (workspace, time_);
      plugin.remove_fullscreen_request (this);
    }
    
    private Clutter.Actor WorkspaceClone (Mutter.MetaWorkspace workspace) {
      Clutter.Group wsp;
      unowned GLib.List<Mutter.Window> windows = plugin.plugin.get_windows ();
      
      wsp = new Clutter.Group ();
      
      foreach (Mutter.Window window in windows)
        {
          if (Mutter.MetaWindow.is_on_all_workspaces (window.get_meta_window ()) ||
              window.get_window_type () == Mutter.MetaCompWindowType.DESKTOP ||
              window.get_workspace () == Mutter.MetaWorkspace.index (workspace))
            {
              Clutter.Actor clone = new Clutter.Clone (window);
              wsp.add_actor (clone);
          
              clone.set_size (window.width, window.height);
              clone.set_position (window.x, window.y);
              
              clone.show ();
              
              if (window.get_window_type () == Mutter.MetaCompWindowType.DESKTOP)
                {
                  clone.lower_bottom ();
                }
            }
        }
      
      return wsp;
    }
    
    private void LayoutWorkspaces (List<Clutter.Actor> clones, Mutter.MetaScreen screen) {
      uint length = clones.length ();
      
      int width  = (int) Math.ceil  (Math.sqrt ((double) length));
      int height = (int) Math.floor (Math.sqrt ((double) length));
      
      Mutter.MetaRectangle rect = {0, 0, 0, 0};
      Mutter.MetaScreen.get_monitor_geometry (screen, 0, rect);
       
      uint item_width = (rect.width - left_padding - right_padding - (width - 1) * spacing) / width;
      float item_scale = (float) item_width / (float) rect.width;
      uint item_height = (uint) (rect.height * item_scale);
      
      for (int y = 0; y < height; y++)
        {
          for (int x = 0; x < width; x++)
            {
              int index = y * width + x;
              Clutter.Actor clone = (Clutter.Actor) clones.nth_data (index);
              
              clone.set_scale (item_scale, item_scale);
              
              clone.x = left_padding + ((item_width + spacing) * x);
              clone.y = top_padding + ((item_height + spacing) * y);
            }
        }
    }
  }
  
}

