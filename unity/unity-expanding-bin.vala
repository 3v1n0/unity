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
  <http://www.gnu.org/licenses/>.
 *
 * Authored by Neil Jagdish Patel <neil.patel@canonical.com>
 */

using GLib;

namespace Unity
{
  public enum ExpandingBinState
  {
    CLOSED,
    UNEXPANDED,
    EXPANDED
  }

  public class ExpandingBin : Ctk.Bin
  {
    public static const int ANIMATION_TIME = 500;
    /*
     * Properties
     */
    public float size_factor {
      get { return _size_factor; }
      set { _size_factor = value; queue_relayout (); }
    }

    public ExpandingBinState bin_state {
      get { return _state; }
      set { _change_state (value); }
    }

    public float unexpanded_height {
      get { return _unexpanded_height; }
      set {
        if (_unexpanded_height != value)
          {
            _unexpanded_height = value;

            if (_state == ExpandingBinState.UNEXPANDED)
              {
                _state = ExpandingBinState.CLOSED;
                _change_state (ExpandingBinState.UNEXPANDED);
              }
          }
      }
    }

    private float             _size_factor = 0.0f;
    private ExpandingBinState _state = ExpandingBinState.CLOSED;
    private ExpandingBinState _old_state = ExpandingBinState.CLOSED;
    private float             _unexpanded_height = 100.0f;
    private float             _expanded_height   = 0.0f;
    private float             _target_height     = 0.0f;

    private float last_height;
    private float last_width;

    /*
     * Construction
     */
    public ExpandingBin ()
    {
      Object ();
    }

    construct
    {
    }

    /*
     * Private Methods
     */
    private override void allocate (Clutter.ActorBox        box,
                                    Clutter.AllocationFlags flags)
    {
      float x = padding.left;
      float y = padding.top;
      float width = Math.floorf (box.x2 - box.x1) - padding.left - padding.right;
      Clutter.ActorBox child_box = Clutter.ActorBox ();
      child_box.x1 = x;
      child_box.x2 = x + width;
      child_box.y1 = y;
      child_box.y2 = y + last_height + ((_target_height - last_height) * _size_factor);

      base.allocate (box, flags);
      get_child ().allocate (child_box, flags);

      last_height = child_box.y2 - child_box.y1;
      last_width = child_box.x2 - child_box.x1;
    }

    private override void get_preferred_height (float     for_width,
                                                out float min_height,
                                                out float nat_height)
    {
      var vpadding = padding.top + padding.bottom;

      get_child ().get_preferred_height (last_width, null, out _expanded_height);
      if (_state == ExpandingBinState.CLOSED)
        {
          _target_height = 0.0f;
        }
      else if (_state == ExpandingBinState.UNEXPANDED)
        {
          _target_height = _unexpanded_height + vpadding;
        }
      else
        {
          _target_height = _expanded_height + vpadding;
        }

      min_height = last_height + ((_target_height - last_height) *_size_factor);
      nat_height = last_height + ((_target_height - last_height) *_size_factor);
    }

    private void _change_state (ExpandingBinState new_state)
    {
      if (_state == new_state)
        return;

      _old_state = _state;
      _state = new_state;

      _size_factor = 0.0f;
      switch (_state)
        {
        case ExpandingBinState.CLOSED:
          var anim = animate (Clutter.AnimationMode.EASE_OUT_QUAD, ANIMATION_TIME,
                   "size_factor", 1.0f,
                   "opacity", 0);
          _target_height = 0.0f;

          anim.completed.connect (() => {
            if (_state == ExpandingBinState.CLOSED)
              hide ();
          });
          break;

        case ExpandingBinState.UNEXPANDED:
          animate (_old_state == ExpandingBinState.CLOSED ? Clutter.AnimationMode.EASE_IN_QUAD
                                                         : Clutter.AnimationMode.EASE_OUT_QUAD,
                   ANIMATION_TIME,
                   "size_factor", 1.0f,
                   "opacity", 255);
          _target_height = _unexpanded_height;
          show ();
          break;

        case ExpandingBinState.EXPANDED:
          animate (Clutter.AnimationMode.EASE_IN_QUAD, ANIMATION_TIME,
                   "size_factor", 1.0f,
                   "opacity", 255);
          get_child ().get_preferred_height (width, null, out _expanded_height);
          _target_height = _expanded_height;
          show ();
          break;

        default:
          warning ("ExpandingBinState %d not supported", _state);
          break;
        }
    }
  }
}
