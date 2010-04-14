/*
 * Copyright (C) 2010 Canonical Ltd
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

namespace Unity.Testing
{
  public class Director : Object
  {
    public Clutter.Stage stage {
        get;
        construct;
    }

    public Director (Clutter.Stage stage)
    {
      Object (stage:stage);
    }

    construct
    {

    }

    private void do_event (Clutter.Actor actor,
                           Clutter.Event event,
                           bool          capture_phase)
    {
      actor.do_event (event, capture_phase);
      while (Gtk.events_pending ())
        Gtk.main_iteration ();
    }

    public void button_press (Clutter.Actor actor,
                              uint32        button,
                              bool          autorelease,
                              float         relative_x,
                              float         relative_y)
    {
      float actor_x, actor_y;
      Clutter.ButtonEvent event = Clutter.ButtonEvent ();

      actor.get_transformed_position (out actor_x, out actor_y);

      event.type = Clutter.EventType.BUTTON_PRESS;
      event.time = Clutter.get_current_event_time ();
      event.flags &= Clutter.EventFlags.FLAG_SYNTHETIC;
      event.stage = actor.get_stage () as Clutter.Stage;
      event.source = actor;
      event.x = actor_x + relative_x;
      event.y = actor_y + relative_y;
      event.button = button;

      do_event (actor, (Clutter.Event)event, false);

      if (autorelease)
        {
          event.type = Clutter.EventType.BUTTON_RELEASE;
          event.time = Clutter.get_current_event_time ();

          do_event (actor, (Clutter.Event)event, false);
        }
    }
  }
}
