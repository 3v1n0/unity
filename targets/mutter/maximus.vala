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
 * Authored by Jason Smith <jason.smith@canonical.com>
 *
 */

namespace Unity
{
  public class Maximus : Object
  {
    public static string user_unmaximize_hint = "maximus-user-unmaximize";

    static string[] default_exclude_classes =
    {
      "Apport-gtk",
      "Bluetooth-properties",
      "Bluetooth-wizard",
      "Download", /* Firefox Download Window */
      "Ekiga",
      "Extension", /* Firefox Add-Ons/Extension Window */
      "Gimp",
      "Global", /* Firefox Error Console Window */
      "Gnome-nettool",
      "Kiten",
      "Kmplot",
      "Nm-editor",
      "Pidgin",
      "Polkit-gnome-authorization",
      "Update-manager",
      "Skype",
      "Toplevel", /* Firefox "Clear Private Data" Window */
      "Transmission"
    };

    public Maximus ()
    {
    }

    construct
    {
    }

    bool window_is_excluded (Mutter.Window window)
    {
      Mutter.MetaCompWindowType type = window.get_window_type ();

      if (type != Mutter.MetaCompWindowType.NORMAL)
        return true;

      unowned Mutter.MetaWindow meta = window.get_meta_window ();

      if (Mutter.MetaWindow.is_maximized (meta) ||
          !Mutter.MetaWindow.allows_resize (meta))
        return true;

      unowned string res_class = Mutter.MetaWindow.get_wm_class (meta);
      foreach (string s in default_exclude_classes)
        if (res_class.contains (s))
          return true;

      void *hint = window.get_data (user_unmaximize_hint);
      if (hint != null)
        return true;

      {
        Clutter.Actor stage = Clutter.Stage.get_default ();

        if (window.width < stage.width * 0.6 || window.width > stage.width ||
            window.height < stage.height * 0.6 || window.height > stage.height ||
            window.width / window.height < 0.6 ||
            window.width / window.height > 2.0)
          {
            return true;
          }

      }

      return false;
    }

    public bool process_window (Mutter.Window window)
    {
      if (window_is_excluded (window))
        return true;

      Mutter.MetaWindow.maximize (window.get_meta_window (), Mutter.MetaMaximizeFlags.HORIZONTAL | Mutter.MetaMaximizeFlags.VERTICAL);

      return true;
    }
  }
}
