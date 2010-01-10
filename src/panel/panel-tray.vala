/* -*- Mode: vala; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
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

namespace Unity.Panel.Tray
{
  public class View : Ctk.Box
  {
    private Manager manager;

    public View ()
    {
      Object (orientation:Ctk.Orientation.HORIZONTAL);
    }

    construct
    {
      this.manager = new Manager ();
    }
  }

  public class Manager : Object
  {
    private Gdk.Screen screen;
    private Gtk.Widget invisible;
    private Gdk.Atom   selection_atom;

    public Manager ()
    {
      ;
    }

    construct
    {
      Gdk.Display      display;
      unowned X.Screen screen;
      string           selection_atom_name;
      int              screen_number;
      uint32           timestamp;

      this.screen = Gdk.Screen.get_default ();

      display = this.screen.get_display ();
      screen = Gdk.x11_screen_get_xscreen (this.screen);
      
      this.invisible = new Gtk.Invisible.for_screen (this.screen);
      this.invisible.realize ();

      this.invisible.add_events (Gdk.EventMask.PROPERTY_CHANGE_MASK
                                 | Gdk.EventMask.STRUCTURE_MASK);

      screen_number = this.screen.get_number ();
      selection_atom_name = @"_NET_SYSTEM_TRAY_S$screen_number";
      this.selection_atom = Gdk.Atom.intern (selection_atom_name, false);

      this.set_properties ();

      timestamp = Gdk.x11_get_server_time (this.invisible.window);

      if (Gdk.selection_owner_set_for_display (display,
                                               this.invisible.window,
                                               this.selection_atom,
                                               timestamp,
                                               true))
        {

        }
      else
        {
          this.invisible.destroy ();
          this.invisible = null;
          this.screen = null;
        }
    }

    private void set_properties ()
    {
      Gdk.Display      display;
      unowned X.Visual xvisual;
      X.Atom           visual_atom;
      uchar[]          data = { 0 };
      X.Atom           orientation_atom;

      /* First the visual atom */
      display = this.invisible.get_display ();
      visual_atom = Gdk.x11_get_xatom_by_name_for_display (display,
                                             "_NET_SYSTEM_TRAY_VISUAL");

      if (this.screen.get_rgba_visual () != null &&
          display.supports_composite ())
        {
          Gdk.Visual gvisual = this.screen.get_rgba_visual ();
          xvisual = _x11_visual_get_xvisual (gvisual);
        }
      else
        {
          Gdk.Colormap colormap;

          colormap = this.screen.get_default_colormap ();
          xvisual = _x11_visual_get_xvisual (colormap.get_visual ());
        }

      data[0] = (uchar)_x_visual_id_from_visual (xvisual);

      unowned X.Display xdisplay = Gdk.x11_display_get_xdisplay (display);
      xdisplay.change_property (Gdk.x11_drawable_get_xid (this.invisible.window),
                         visual_atom,
                         (X.Atom)32, 32,
                         X.PropMode.Replace,
                         data, 1);

      /* Now the orientation atom */
      orientation_atom = Gdk.x11_get_xatom_by_name_for_display (display,
                                         "_NET_SYSTEM_TRAY_ORIENTATION");
      data[0] = 0; /* Orientation */

      xdisplay.change_property (Gdk.x11_drawable_get_xid (this.invisible.window),
                                orientation_atom,
                                (X.Atom)6, /* XA_CARDINAL */
                                32,
                                X.PropMode.Replace,
                                data, 1);
    }
  }
}
