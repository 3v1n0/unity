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
    static string[] default_exclude_classes = 
    {
      "Apport-gtk",
      "Bluetooth-properties",
      "Bluetooth-wizard",
      "Download", /* Firefox Download Window */
      "Ekiga",
      "Extension", /* Firefox Add-Ons/Extension Window */
      "Gcalctool",
      "Gimp",
      "Global", /* Firefox Error Console Window */
      "Gnome-dictionary",
      "Gnome-launguage-selector",
      "Gnome-nettool",
      "Gnome-volume-control",
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
    
    bool window_is_excluded (Wnck.Window window)
    {
      Wnck.WindowType type = window.get_window_type ();
      
      if (type != Wnck.WindowType.NORMAL)
        return true;
      
      if (window.is_fullscreen ())
        return true;
      
      Wnck.WindowActions actions = window.get_actions ();
     
      if (!(((actions & Wnck.WindowActions.RESIZE) == Wnck.WindowActions.RESIZE) && 
            ((actions & Wnck.WindowActions.MAXIMIZE_HORIZONTALLY) == Wnck.WindowActions.MAXIMIZE_HORIZONTALLY) && 
            ((actions & Wnck.WindowActions.MAXIMIZE_VERTICALLY) == Wnck.WindowActions.MAXIMIZE_VERTICALLY) && 
            ((actions & Wnck.WindowActions.MAXIMIZE) == Wnck.WindowActions.MAXIMIZE)))
        {
          return true;
        }
      
      unowned Wnck.ClassGroup group = window.get_class_group ();
      
      if (group is Wnck.ClassGroup)
        {
          unowned string name = group.get_name ();
          unowned string res_class = group.get_res_class ();
          foreach (string s in default_exclude_classes)
            if (name.contains (s) || res_class.contains (s))
              return true;
        }
      
      return false;
    }
    
    public bool process_window (Mutter.Window window)
    {
      ulong xid = (ulong) Mutter.MetaWindow.get_xwindow (window.get_meta_window ());
      
      Wnck.Window wnck_window = Wnck.Window.get (xid);
      
      if (!(wnck_window is Wnck.Window) || window_is_excluded (wnck_window))
        return true;
      
      wnck_window.maximize ();
      
      return true;
    }
  }
}
