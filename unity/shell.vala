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
 * Authored by Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */

namespace Unity
{
  public enum ShellMode {
    UNDERLAY,
    OVERLAY
  }
  public enum dnd_targets {
    TARGET_INT32,
    TARGET_STRING,
    TARGET_URL,
    TARGET_OTHER
  }

  public interface Shell : Object
  {
    public abstract bool          menus_swallow_events {get;}

    public abstract uint32        get_current_time ();
    public abstract ShellMode     get_mode ();
    public abstract Clutter.Stage get_stage ();
    public abstract void          show_unity ();
    public abstract int           get_indicators_width ();
    public abstract int           get_launcher_width_foobar (); // _foobar is used to avoid a symbol-clash
    public abstract int           get_panel_height_foobar ();   // with unity/targets/mutter/main.c
    public abstract void          ensure_input_region ();
    public abstract void          add_fullscreen_request (Object o);
    public abstract bool          remove_fullscreen_request (Object o);
    public abstract void          grab_keyboard (bool grab, uint32 timestamp);
    public abstract void          about_to_show_places ();

    public abstract void          close_xids (Array<uint32> xids);
    public abstract void          show_window (uint32 xid);

    public abstract void          expose_xids (Array<uint32> xids);
    public abstract void          stop_expose ();

    public signal   void need_new_icon_cache ();
    public signal   void indicators_changed (int width);
  }

  public Shell? global_shell; // our global shell
}
