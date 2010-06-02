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

    public abstract ShellMode     get_mode ();
    public abstract Clutter.Stage get_stage ();
    public abstract void          show_unity ();
    public abstract int           get_indicators_width ();
    public abstract int           get_launcher_width ();
    public abstract int           get_panel_height ();
    public abstract void          ensure_input_region ();
    public abstract void          add_fullscreen_request (Object o);
    public abstract bool          remove_fullscreen_request (Object o);
    public abstract void          grab_keyboard (bool grab, uint32 timestamp);
    public abstract void          show_window_picker ();

    public abstract void          close_xids (Array<uint32> xids);
    public abstract void          show_window (uint32 xid);

		public abstract void					expose_xids (Array<uint32> xids);
		public abstract void					stop_expose ();

    public signal   void need_new_icon_cache ();
    public signal   void indicators_changed (int width);
  }

  public Shell? global_shell; // our global shell
}
