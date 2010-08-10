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

    public uint32 x;
    public uint32 y;

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
      ret += "\tX Root     : %u\n".printf (x);
      ret += "\tY Root     : %u\n".printf (y);
      ret += "\tRootWin    : %u\n".printf ((uint32)root_window);
      ret += "\tEventWin   : %u\n".printf ((uint32)event_window);
      ret += "\tChildWin   : %u\n".printf ((uint32)child_window);

      switch (type)
        {
        case Type.PINCH:
          ret += "\tDelta      : %f\n".printf (pinch_event.delta);
          break;

        case Type.TAP:
          ret += "\tDuration   : %u\n".printf (tap_event.duration);
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
    public float delta;
  }

  public class PinchEvent
  {
    public float delta;
  }

  public class TapEvent
  {
    public int32 duration;
  }
}
