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

namespace Unity
{
  public class ExposeClone : Clutter.Group
  {
    private Clutter.Clone clone;
    private Clutter.Actor darken_box;
    private bool hovered;

    public unowned Clutter.Actor source { get; private set; }

    public uint8 hovered_opacity { get; set; }
    public uint8 unhovered_opacity { get; set; }

    uint8 _darken;
    public uint8 darken {
      get { return _darken; }
      set {
        _darken = value;
        if (!hovered && darken_box is Clutter.Actor)
          darken_box.opacity = darken;
      }
    }

    public ExposeClone (Clutter.Actor source)
    {
      darken = 0;
      hovered_opacity = 255;
      unhovered_opacity = 255;

      this.source = source;
      
      if (source is Mutter.Window)
        clone = new Clutter.Clone ((source as Mutter.Window).get_texture ());
      else
        clone = new Clutter.Clone (source);

      add_actor (clone);
      clone.show ();
      clone.reactive = true;
      clone.set_position (0, 0);

      darken_box = new Clutter.Rectangle.with_color ({0, 0, 0, 255});
      add_actor (darken_box);
      darken_box.raise_top ();

      darken_box.set_position (0, 0);
      darken_box.set_size (source.width, source.height);

      darken_box.opacity = darken;
    }

    construct
    {
      this.enter_event.connect (this.on_mouse_enter);
      this.leave_event.connect (this.on_mouse_leave);
    }

    private bool on_mouse_enter (Clutter.Event evnt)
    {
      hovered = true;
      opacity = hovered_opacity;
      darken_box.opacity = 0;
      return false;
    }

    private bool on_mouse_leave (Clutter.Event evnt)
    {
      hovered = false;
      opacity = unhovered_opacity;
      darken_box.opacity = darken;
      return false;
    }
  }

  public class ExposeManager : Object
  {
    private List<ExposeClone> exposed_windows;
    private Clutter.Group expose_group;
    private Plugin owner;
    private Clutter.Stage stage;
    private Launcher.Launcher launcher;

    public bool expose_showing { get; private set; }
    public bool coverflow { get; set; }

    public int left_buffer { get; set; }
    public int right_buffer { get; set; }
    public int top_buffer { get; set; }
    public int bottom_buffer { get; set; }

    public uint8 hovered_opacity { get; set; }
    public uint8 unhovered_opacity { get; set; }

    public uint8 darken { get; set; }

    private uint coverflow_index;

    private ExposeClone? last_selected_clone = null;

    private bool menu_in_hover_close_state = false;

    public ExposeManager (Plugin plugin, Launcher.Launcher launcher)
    {
      this.launcher = launcher;
      this.owner = plugin;
      this.exposed_windows = new List<ExposeClone> ();
      this.stage = (Clutter.Stage)plugin.get_stage ();

      hovered_opacity = 255;
      unhovered_opacity = 255;
      darken = 0;
    }

    construct
    {
    }

    public void start_expose (SList<Clutter.Actor> windows)
    {
      var controller = Launcher.QuicklistController.get_current_menu ();
      if (controller.is_menu_open ())
        {
          controller.get_view ().destroy.connect (this.end_expose);
          this.menu_in_hover_close_state = controller.get_view ().get_close_on_leave ();
        }
      exposed_windows = new List<ExposeClone> ();

      if (expose_group != null)
        expose_group.destroy ();
      expose_group = new Clutter.Group ();

      Clutter.Actor window_group = owner.plugin.get_normal_window_group ();

      (window_group as Clutter.Container).add_actor (expose_group);
      expose_group.raise_top ();
      expose_group.show ();

      foreach (Clutter.Actor actor in windows)
        {
          if (!(actor is Mutter.Window) ||
               ((actor as Mutter.Window).get_window_type () != Mutter.MetaCompWindowType.NORMAL &&
                (actor as Mutter.Window).get_window_type () != Mutter.MetaCompWindowType.DIALOG &&
                (actor as Mutter.Window).get_window_type () != Mutter.MetaCompWindowType.MODAL_DIALOG))
            continue;

          ExposeClone clone = new ExposeClone (actor);
          clone.set_position (actor.x, actor.y);
          clone.set_size (actor.width, actor.height);
          exposed_windows.append (clone);
          clone.reactive = true;

          expose_group.add_actor (clone);

          clone.hovered_opacity = hovered_opacity;
          clone.unhovered_opacity = unhovered_opacity;
          clone.opacity = unhovered_opacity;
          clone.darken = darken;

          clone.enter_event.connect (() => {
                var ql_controller = Launcher.QuicklistController.get_current_menu ();
                if (ql_controller.state == Launcher.QuicklistControllerState.MENU
                    && this.menu_in_hover_close_state)
                  {
                    ql_controller.get_view ().set_close_on_leave (false);
                  }
                return false;
              });

          clone.leave_event.connect (() => {
                var ql_controller = Launcher.QuicklistController.get_current_menu ();
                if (ql_controller.state == Launcher.QuicklistControllerState.MENU
                    && this.menu_in_hover_close_state)
                  {
                    ql_controller.get_view ().set_close_on_leave (true);
                  }
                return false;
              });
        }

      unowned GLib.List<Mutter.Window> mutter_windows = owner.plugin.get_windows ();
      foreach (Mutter.Window w in mutter_windows)
        {
            if (w.get_window_type () == Mutter.MetaCompWindowType.DESKTOP)
              continue;

            (w as Clutter.Actor).reactive = false;
            (w as Clutter.Actor).opacity = 0;
        }
      coverflow_index = 0;

      if (coverflow)
        position_windows_coverflow (exposed_windows, exposed_windows.nth_data (coverflow_index));
      else
        position_windows_on_grid (exposed_windows);

      expose_showing = true;

      owner.add_fullscreen_request (this);
      stage.captured_event.connect (on_stage_captured_event);
    }

    public void end_expose ()
    {
      if (!expose_showing)
        return;
      
      var controller = Launcher.QuicklistController.get_current_menu ();
      if (controller.is_menu_open ())
        {
          controller.get_view ().destroy.disconnect (this.end_expose);
          controller.state = Launcher.QuicklistControllerState.CLOSED;
        }

      unowned GLib.List<Mutter.Window> mutter_windows = owner.plugin.get_windows ();
      foreach (Mutter.Window window in mutter_windows)
        {
          bool exposed = false;
          foreach (ExposeClone clone in exposed_windows)
            {
              if (clone.source == window)
                {
                  exposed = true;
                  break;
                }
            }

          if (!exposed)
            window.animate (Clutter.AnimationMode.EASE_IN_OUT_SINE, 250, "opacity", 255);
          window.reactive = true;
        }

      foreach (Clutter.Actor actor in exposed_windows)
        restore_window_position (actor);

      if (this.last_selected_clone is ExposeClone &&
          this.last_selected_clone.source is Mutter.Window)
        {
          ExposeClone clone = this.last_selected_clone;
          uint32 time_;

          clone.raise_top ();
          unowned Mutter.MetaWindow meta = (clone.source as Mutter.Window).get_meta_window ();

          time_ = Mutter.MetaDisplay.get_current_time (Mutter.MetaWindow.get_display (meta));
          Mutter.MetaWorkspace.activate (Mutter.MetaWindow.get_workspace (meta), time_);
          Mutter.MetaWindow.activate (meta, time_);

          this.last_selected_clone = null;
        }

      expose_showing = false;
      owner.remove_fullscreen_request (this);
      stage.captured_event.disconnect (on_stage_captured_event);
    }

    void position_windows_coverflow (List<Clutter.Actor> windows, Clutter.Actor active)
    {
      Clutter.Actor last = null;

      int middle_size = (int) (stage.width * 0.8f);
      int width = (int) stage.width - left_buffer - right_buffer;
      int slice_width = width / 10;

      int middle_y = (int) stage.height / 2;
      int middle_x = left_buffer + width / 2;

      int middle_index = windows.index (active);

      float scale = float.min (1f, (stage.height / 2) / float.max (active.height, active.width));
      scale = 1f;

      active.set_anchor_point_from_gravity (Clutter.Gravity.CENTER);
      active.animate (Clutter.AnimationMode.EASE_IN_OUT_SINE, 250,
                      "x", (float) middle_x,
                      "y", (float) middle_y,
                      "depth", stage.width * -0.7,
                      "scale-x", scale,
                      "scale-y", scale,
                      "rotation-angle-y", 0f);
      active.raise_top ();

      last = active;
      /* left side */
      int current_x = middle_x - middle_size;
      for (int i = middle_index - 1; i >= 0; i--)
        {
          Clutter.Actor actor = windows.nth_data (i);
          actor.set_anchor_point_from_gravity (Clutter.Gravity.CENTER);
          actor.lower (last);

          scale = float.min (1f, (stage.height / 2) / float.max (actor.height, actor.width));
          scale = 1f;

          actor.animate (Clutter.AnimationMode.EASE_IN_OUT_SINE, 250,
                          "x", (float) current_x,
                          "y", (float) middle_y,
                          "depth", stage.width * -0.7,
                          "scale-x", scale,
                          "scale-y", scale,
                          "rotation-angle-y", 60f);
          current_x -= slice_width;
          last = actor;
        }

      last = active;
      /* right side */
      current_x = middle_x + middle_size;
      for (int i = middle_index + 1; i < windows.length (); i++)
        {
          Clutter.Actor actor = windows.nth_data (i);
          actor.set_anchor_point_from_gravity (Clutter.Gravity.CENTER);
          actor.lower (last);

          scale = float.min (1f, (stage.height / 2) / float.max (actor.height, actor.width));
          scale = 1f;

          actor.animate (Clutter.AnimationMode.EASE_IN_OUT_SINE, 250,
                          "x", (float) current_x,
                          "y", (float) middle_y,
                          "depth", stage.width * -0.7,
                          "scale-x", scale,
                          "scale-y", scale,
                          "rotation-angle-y", -60f);
          current_x += slice_width;
          last = actor;
        }
    }

    int direct_comparison (void* a, void* b)
    {
      if (a > b)
        return 1;
      else if (a < b)
        return  -1;
      return 0;
    }

    void position_windows_on_grid (List<Clutter.Actor> _windows)
    {
      List<Clutter.Actor> windows = _windows.copy ();
      windows.sort ((CompareFunc) direct_comparison);

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

    void handle_event_coverflow (Clutter.Event event)
    {
      if (event.type == Clutter.EventType.KEY_RELEASE)
        {
          uint16 keycode = event.get_key_code ();

          if (keycode == 113 && coverflow_index > 0)
            {
              coverflow_index--;
            }
          else if (keycode == 114 && coverflow_index < exposed_windows.length () - 1)
            {
              coverflow_index++;
            }
          else if (keycode == 36)
            {
              ExposeClone clone = exposed_windows.nth_data (coverflow_index);
              clone.raise_top ();
              unowned Mutter.MetaWindow meta = (clone.source as Mutter.Window).get_meta_window ();
              Mutter.MetaWorkspace.activate (Mutter.MetaWindow.get_workspace (meta), event.get_time ());
              Mutter.MetaWindow.activate (meta, event.get_time ());
              this.end_expose ();
            }

          position_windows_coverflow (exposed_windows, exposed_windows.nth_data (coverflow_index));
        }
    }

    void handle_event_expose (Clutter.Event event, Clutter.Actor actor)
    {
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
    }

    void pick_window (Clutter.Event event, Clutter.Actor actor)
    {
      while (actor.get_parent () != null && !(actor is ExposeClone))
        actor = actor.get_parent ();

      ExposeClone clone = actor as ExposeClone;
      if (clone != null && clone.source is Mutter.Window)
        {
          this.last_selected_clone= clone;
        }
      else
        {
          this.last_selected_clone = null;
        }
    }

    bool on_stage_captured_event (Clutter.Event event)
    {
      if (event.type == Clutter.EventType.ENTER || event.type == Clutter.EventType.LEAVE)
        return false;

      bool event_over_menu = false;

      float x, y;
      event.get_coords (out x, out y);

      unowned Clutter.Actor actor = this.stage.get_actor_at_pos (Clutter.PickMode.REACTIVE, (int) x, (int) y);

      Clutter.Actor? menu = null;
      if (Unity.Launcher.QuicklistController.get_current_menu ().is_menu_open ())
        menu = Unity.Launcher.QuicklistController.get_current_menu ().get_view ();
      if (menu != null)
        {
          if (x > menu.x && x < menu.x + menu.width && y > menu.y && y < menu.y + menu.height)
            event_over_menu = true;
        }

      if (event.type == Clutter.EventType.BUTTON_PRESS && !event_over_menu)
        pick_window (event, actor);

      if (coverflow)
        handle_event_coverflow (event);
      else
        handle_event_expose (event, actor);

      return !event_over_menu;
    }
  }
}
