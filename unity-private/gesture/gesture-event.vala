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

using X;

namespace Unity.Gesture
{
  public enum Type
  {
    NONE,
    PAN,
    PINCH,
    ROTATE,
    TAP
  }

  public enum State
  {
    BEGAN,
    CONTINUED,
    ENDED
  }
  
  public class Event
  {
    public Type   type;

    public uint32 device_id;
    public uint32 gesture_id;
    public uint32 timestamp;
    public State  state;
    public int32  fingers;

    public float root_x;
    public float root_y;

    public Window root_window;
    public Window event_window;
    public Window child_window;

    public PanEvent   pan_event;
    public PinchEvent pinch_event;
    public TapEvent   tap_event;

    public string to_string ()
    {
      string ret = "";

      ret += type_to_string (type) + " Gesture\n";
      ret += "\tDevice ID  : %u\n".printf (device_id);
      ret += "\tGesture ID : %u\n".printf (gesture_id);
      ret += "\tTimestamp  : %u\n".printf (timestamp);
      ret += "\tState      : " + state_to_string (state) + "\n";
      ret += "\tFingers    : %u\n".printf (fingers);
      ret += "\tX Root     : %f\n".printf (root_x);
      ret += "\tY Root     : %f\n".printf (root_y);
      ret += "\tRootWin    : %u\n".printf ((uint32)root_window);
      ret += "\tEventWin   : %u\n".printf ((uint32)event_window);
      ret += "\tChildWin   : %u\n".printf ((uint32)child_window);

      switch (type)
        {
        case Type.PAN:
          ret += "\tDeltaX     : %f\n".printf (pan_event.delta_x);
          ret += "\tDeltaY     : %f\n".printf (pan_event.delta_y);
          ret += "\tVelocityX  : %f\n".printf (pan_event.velocity_x);
          ret += "\tVelocityY  : %f\n".printf (pan_event.velocity_y);
          ret += "\tX          : %f\n".printf (pan_event.x);
          ret += "\tY          : %f\n".printf (pan_event.y);
          ret += "\tCurNFingers: %f\n".printf (pan_event.current_n_fingers);
          break;
        case Type.PINCH:
          ret += "\tRadiusDelta: %f\n".printf (pinch_event.radius_delta);
          ret += "\tRadiusVeloc: %f\n".printf (pinch_event.radius_velocity);
          ret += "\tRadius     : %f\n".printf (pinch_event.radius);
          ret += "\tBoundingX1 : %f\n".printf (pinch_event.bounding_box_x1);
          ret += "\tBoundingY1 : %f\n".printf (pinch_event.bounding_box_y1);
          ret += "\tBoundingX2 : %f\n".printf (pinch_event.bounding_box_x2);
          ret += "\tBoundingY2 : %f\n".printf (pinch_event.bounding_box_y2);
          break;

        case Type.TAP:
          ret += "\tDuration   : %f\n".printf (tap_event.duration); 
          ret += "\tX2         : %f\n".printf (tap_event.x2);
          ret += "\tY2         : %f\n".printf (tap_event.y2);
          break;
        
        default:
          break;
        }

      return ret;
    }

    public static string type_to_string (Type type)
    {
      if (type == Type.PAN)
        return "Pan";
      else if (type == Type.PINCH)
        return "Pinch";
      else if (type == Type.ROTATE)
        return "Rotate";
      else if (type == Type.TAP)
        return "Tap";
      else
        return "Unknown";
    }

    public static string state_to_string (State state)
    {
      if (state == State.BEGAN)
        return "Began";
      else if (state == State.CONTINUED)
        return "Continued";
      else if (state == State.ENDED)
        return "Ended";
      else
        return "Unknown";
    }
  }

  public class PanEvent
  {
    public float delta_x;
    public float delta_y;
    public float velocity_x;
    public float velocity_y;
    public float x;
    public float y;
    public float current_n_fingers; /* For 3-to-2 drags, for example */
  }

  public class PinchEvent
  {
    public float radius_delta;
    public float radius_velocity;
    public float radius;
    public float bounding_box_x1;
    public float bounding_box_y1;
    public float bounding_box_x2;
    public float bounding_box_y2;
  }

  public class TapEvent
  {
    public float duration;
    public float x2;
    public float y2;
  }
}
