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
  public class Entry : Ctk.Text
  {
    public Entry ()
    {
      Object ();
    }

    construct
    {
      this.reactive = true;
      this.editable = true;
      this.selectable = true;
      this.activatable = true;
      this.single_line_mode = true;

      this.button_press_event.connect (this.on_button_press_event);
      this.activate.connect (this.on_activate);
    }

    private void on_activate ()
    {
      Unity.global_shell.grab_keyboard (false,
                                        Clutter.get_current_event_time ());
    }

    private bool on_button_press_event (Clutter.Event event)
    {
      Unity.global_shell.grab_keyboard (true, event.button.time);

      this.get_stage ().captured_event.connect (this.on_stage_captured_event);

      return false;
    }

    private bool on_stage_captured_event (Clutter.Event event)
    {
      if (event.type == Clutter.EventType.BUTTON_PRESS)
      {
        Clutter.Stage stage = this.get_stage () as Clutter.Stage;

        var actor = stage.get_actor_at_pos (Clutter.PickMode.REACTIVE,
                                            (int)event.button.x,
                                            (int)event.button.y);

        if (actor != this)
          {
            Unity.global_shell.grab_keyboard (false, event.button.time);
            stage.captured_event.disconnect (this.on_stage_captured_event);
          }
      }

      return false;
    }
  }
}
