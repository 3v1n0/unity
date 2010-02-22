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
using Unity;
using Unity.Quicklauncher;
using Unity.Quicklauncher.Models;

namespace Unity
{
  public class ExposeClone : Clutter.Group
  {
    private Clutter.Clone clone;
    
    public Mutter.Window source { get; private set; }

    public uint8 hovered_opacity { get; set; }
    public uint8 unhovered_opacity { get; set; }
    
    public ExposeClone (Mutter.Window source)
    {
      this.source = source;
      clone = new Clutter.Clone (source);
      
      add_actor (clone);
      clone.show ();
      clone.set_position (0, 0);
    }
    
    construct
    {
      this.enter_event.connect (this.on_mouse_enter);
      this.leave_event.connect (this.on_mouse_leave);
    }
    
    private bool on_mouse_enter (Clutter.Event evnt)
    {
      opacity = hovered_opacity;
      return false;
    }
    
    private bool on_mouse_leave (Clutter.Event evnt)
    {
      opacity = unhovered_opacity;
      return false;
    }
  }
  
  public class ExposeManager : Object
  {
    private List<ExposeClone> exposed_windows;
    private Plugin owner;
    private Clutter.Stage stage;
    private Quicklauncher.View quicklauncher;
    
    public bool expose_showing { get; private set; }
    public bool coverflow { get; set; }
    
    public int left_buffer { get; set; }
    public int right_buffer { get; set; }
    public int top_buffer { get; set; }
    public int bottom_buffer { get; set; }
    
    public uint8 hovered_opacity { get; set; }
    public uint8 unhovered_opacity { get; set; }
    
    public ExposeManager (Plugin plugin, Quicklauncher.View quicklauncher)
    {
      this.quicklauncher = quicklauncher;
      this.owner = plugin;
      this.exposed_windows = new List<ExposeClone> ();
      this.stage = (Clutter.Stage)plugin.get_stage ();
      
      hovered_opacity = 255;
      unhovered_opacity = 255;
    }
    
    construct
    {
    }
    
    public void start_expose (SList<Wnck.Window> windows)
    {
      exposed_windows = new List<ExposeClone> ();

      unowned GLib.List<Mutter.Window> mutter_windows = owner.plugin.get_windows ();
      foreach (Mutter.Window w in mutter_windows)
        {
          bool keep = false;
          ulong xid = (ulong) Mutter.MetaWindow.get_xwindow (w.get_meta_window ());
          foreach (Wnck.Window window in windows)
            {
              if (window.get_xid () == xid)
                {
                  keep = true;
                  break;
                }
            }

          if (keep)
            {
              ExposeClone clone = new ExposeClone (w);
              clone.set_position (w.x, w.y);
              clone.set_size (w.width, w.height);
              clone.opacity = w.opacity;
              exposed_windows.append (clone);
              clone.reactive = true;

              unowned Clutter.Container container = w.get_parent () as Clutter.Container;

              container.add_actor (clone);
              (clone as Clutter.Actor).raise (w);
              
              clone.hovered_opacity = hovered_opacity;
              clone.unhovered_opacity = unhovered_opacity;
              clone.opacity = unhovered_opacity;
            }

            if (w.get_window_type () == Mutter.MetaCompWindowType.DESKTOP)
              {
                exposed_windows.reverse ();
                foreach (Clutter.Actor actor in exposed_windows)
                  {
                    actor.raise (w);
                  }
                exposed_windows.reverse ();
                continue;
              }
            (w as Clutter.Actor).reactive = false;
            (w as Clutter.Actor).animate (Clutter.AnimationMode.EASE_IN_SINE, 80, "opacity", 0);
        }

      if (coverflow)
        position_windows_coverflow (exposed_windows, exposed_windows.nth_data (0));
      else
        position_windows_on_grid (exposed_windows);

      expose_showing = true;
      
      owner.add_fullscreen_request (this);
      stage.captured_event.connect (on_stage_captured_event);
    }
    
    public void end_expose ()
    {
      if (quicklauncher.manager.active_launcher != null)
        quicklauncher.manager.active_launcher.close_menu ();

      unowned GLib.List<Mutter.Window> mutter_windows = owner.plugin.get_windows ();
      foreach (Mutter.Window window in mutter_windows)
        {
          window.opacity = 255;
          window.reactive = true;
        }

      foreach (Clutter.Actor actor in exposed_windows)
        restore_window_position (actor);

      expose_showing = false;
      owner.remove_fullscreen_request (this);
    }
    
    void position_windows_coverflow (List<Clutter.Actor> windows, Clutter.Actor active)
    {
      int middle_size = (int) stage.width / 3;
      int width = (int) stage.width - left_buffer - right_buffer - middle_size * 2 - 50;
      int slice_width = width / (int) windows.length ();
      
      int current_x = left_buffer + slice_width / 2 + 50;
      int current_y = (int) stage.height / 2; /* fixme */
      
      bool active_seen = false;
      float y_angle;
      
      Clutter.Actor last = null;
      foreach (Clutter.Actor actor in windows)
        {
          actor.set_anchor_point_from_gravity (Clutter.Gravity.CENTER);
          if (actor == active)
            {
              current_x += middle_size;
              /* Our main window */
              y_angle = 0f;
              active_seen = true;
              
              if (last == null)
                actor.raise_top ();
              else
                actor.raise (last);
            }
          else if (active_seen)
            {
              /* right side */
              y_angle = -50f;
              actor.lower (last);
            }
          else
            {
              /* left side */
              y_angle = 50f;
              if (last == null)
                actor.raise_top ();
              else
                actor.raise (last);
            }
          
          last = actor;
          actor.animate (Clutter.AnimationMode.EASE_IN_OUT_SINE, 250,
                                                 "x", (float) current_x,
                                                 "y", (float) current_y,
                                                 "scale-x", 0.5f,
                                                 "scale-y", 0.5f,
                                                 "rotation-angle-y", y_angle);
          
          current_x += slice_width;
          
          if (actor == active)
            current_x += middle_size;
        }
    }
    
    void position_windows_on_grid (List<Clutter.Actor> _windows)
    {
      List<Clutter.Actor> windows = _windows.copy ();
      int count = (int) windows.length ();

      int cols = (int) Math.ceil (Math.sqrt (count));

      int rows = 1;

      while (cols * rows < count)
        rows++;

      int boxWidth = (int) ((stage.width - left_buffer - right_buffer) / cols);
      int boxHeight = (int) ((stage.height - top_buffer - bottom_buffer) / rows);

      for (int row = 0; row < rows; row++)
        {
          if (row == rows - 1)
            {
              /* Last row, time to perform centering as needed */
              boxWidth = (int) ((stage.width - left_buffer - right_buffer) / windows.length ());
            }
            
          for (int col = 0; col < cols; col++)
            {
              if (windows.length () == 0)
                return;

              int centerX = (boxWidth / 2 + boxWidth * col + left_buffer);
              int centerY = (boxHeight / 2 + boxHeight * row + top_buffer);

              Clutter.Actor window = null;
              // greedy layout
              foreach (Clutter.Actor actor in windows)
                {
                  if (window == null)
                    {
                      window = actor;
                      continue;
                    }

                  double window_distance = Math.sqrt (
                    Math.fabs (centerX - (window.x + window.width / 2)) +
                    Math.fabs (centerY - (window.y + window.height / 2))
                  );
                  double actor_distance = Math.sqrt (
                    Math.fabs (centerX - (actor.x + actor.width / 2)) +
                    Math.fabs (centerY - (actor.y + actor.height / 2))
                  );

                  if (actor_distance < window_distance)
                    window = actor;
                }

              windows.remove (window);

              int windowX = centerX - (int) window.width / 2;
              int windowY = centerY - (int) window.height / 2;

              float scale = float.min (float.min (1, (boxWidth - 20) / window.width), float.min (1, (boxHeight - 20) / window.height));

              window.set ("scale-gravity", Clutter.Gravity.CENTER);
              window.animate (Clutter.AnimationMode.EASE_IN_OUT_SINE, 250,
                                                 "x", (float) windowX,
                                                 "y", (float) windowY,
                                                 "scale-x", scale,
                                                 "scale-y", scale);
            }
        }
    }
    
    private void restore_window_position (Clutter.Actor actor)
    {
      if (!(actor is ExposeClone))
        return;

      actor.set_anchor_point_from_gravity (Clutter.Gravity.NORTH_WEST);
      Clutter.Actor window = (actor as ExposeClone).source;

      uint8 opacity = 0;
      if ((window as Mutter.Window).showing_on_its_workspace () && 
          (window as Mutter.Window).get_workspace () == Mutter.MetaScreen.get_active_workspace_index (owner.plugin.get_screen ()))
        opacity = 255;
        
      actor.set ("scale-gravity", Clutter.Gravity.CENTER);
      Clutter.Animation anim = actor.animate (Clutter.AnimationMode.EASE_IN_OUT_SINE, 250,
                                         "scale-x", 1f,
                                         "scale-y", 1f,
                                         "opacity", opacity,
                                         "x", window.x,
                                         "y", window.y);

      window.opacity = 0;

      anim.completed.connect (() => {
        actor.destroy ();
        window.opacity = 255;
      });
    }
    
    bool on_stage_captured_event (Clutter.Event event)
    {
      if (event.type == Clutter.EventType.ENTER || event.type == Clutter.EventType.LEAVE)
        return false;

      bool event_over_menu = false;

      float x, y;
      event.get_coords (out x, out y);

      unowned Clutter.Actor actor = this.stage.get_actor_at_pos (Clutter.PickMode.REACTIVE, (int) x, (int) y);

      unowned Clutter.Actor menu = null;
      if (Unity.Quicklauncher.active_menu != null)
        menu = Unity.Quicklauncher.active_menu.menu as Clutter.Actor;
      if (menu != null)
        {
          if (x > menu.x && x < menu.x + menu.width && y > menu.y && y < menu.y + menu.height)
            event_over_menu = true;
        }

      if (event.type == Clutter.EventType.BUTTON_RELEASE && event.get_button () == 1)
        {
          while (actor.get_parent () != null && !(actor is ExposeClone))
            actor = actor.get_parent ();

          ExposeClone clone = actor as ExposeClone;
          if (clone != null && clone.source is Mutter.Window)
            {
              clone.raise_top ();
              unowned Mutter.MetaWindow meta = (clone.source as Mutter.Window).get_meta_window ();
              Mutter.MetaWorkspace.activate (Mutter.MetaWindow.get_workspace (meta), event.get_time ());
              Mutter.MetaWindow.activate (meta, event.get_time ());
            }
          this.stage.captured_event.disconnect (on_stage_captured_event);
          this.end_expose ();
        }

      return !event_over_menu;
    }
  }
}
