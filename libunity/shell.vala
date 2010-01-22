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
    public abstract ShellMode     get_mode ();
    public abstract Clutter.Stage get_stage ();
    public abstract void          show_unity ();

    public abstract void          set_in_menu (bool is_in_menu);
  }

  public Shell? global_shell; // our global shell
}
