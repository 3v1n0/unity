/*
 * Copyright (C) 2010 Canonical, Ltd.
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License
 * version 3.0 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License version 3.0 for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Authored by Gordon Allott <gord.allott@canonical.com>
 */


namespace Unity.Drag
{
  private class View : Object
  {
    public Clutter.Stage stage {get; construct;}
    Clutter.Actor? hooked_actor;
    float offset_x;
    float offset_y;

    public signal void motion (float x, float y);
    public signal void end (float x, float y);

    public View (Clutter.Stage stage)
    {
      Object (stage:stage);
    }

    construct
    {

    }
    public void hook_actor_to_cursor (Clutter.Actor actor, float offset_x, float offset_y)
    {
      if (this.hooked_actor is Clutter.Actor) {
        warning ("attempted to hook a new actor to dnd without unhooking previous actor");
        this.unhook_actor ();
      }
      float x, y;
      this.offset_x = offset_x;
      this.offset_y = offset_y;

      this.hooked_actor = new Clutter.Clone (actor);
      this.stage.add_actor (this.hooked_actor);

      actor.get_transformed_position (out x, out y);
      this.hooked_actor.set_position (x, y);
      this.hooked_actor.set_width (actor.get_width ());
      this.hooked_actor.set_height (actor.get_height ());
      this.hooked_actor.show ();

      this.stage.captured_event.connect (this.captured_event);
    }

    private bool captured_event (Clutter.Event event)
    {
      if (event.type == Clutter.EventType.MOTION)
        {
          this.on_motion_event (event);
          return true;
        }
      if (event.type == Clutter.EventType.BUTTON_RELEASE)
        {
          this.on_release_event (event);
          return true;
        }
      return false;
    }

    public void unhook_actor ()
    {
      if (this.hooked_actor == null) return;
      this.stage.remove_actor (this.hooked_actor);
      this.stage.motion_event.disconnect (this.on_motion_event);
      this.hooked_actor = null;
    }

    private bool on_motion_event (Clutter.Event event)
    {
      if (!(this.hooked_actor is Clutter.Actor)) {
        this.stage.captured_event.disconnect (this.captured_event);
        return false;
      }
      this.hooked_actor.set_position (event.motion.x - this.offset_x,
                                      event.motion.y - this.offset_y);
      this.motion (event.motion.x, event.motion.y);
      this.hooked_actor.set_opacity (255);
      this.hooked_actor.show ();
      return false;
    }

    private bool on_release_event (Clutter.Event event)
    {
      if (!(this.hooked_actor is Clutter.Actor)) {
        this.stage.button_release_event.disconnect (this.on_release_event);
        return false;
      }
      this.unhook_actor ();
      this.end (event.button.x, event.button.y);
      return false;
    }
  }
}

