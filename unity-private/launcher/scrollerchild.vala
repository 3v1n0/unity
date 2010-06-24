/*
 *      scrollerchild.vala.vala
 *      Copyright (C) 2010 Canonical Ltd
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 *
 *
 *      Authored by Gordon Allott <gord.allott@canonical.com>
 */


namespace Unity.Launcher
{
  public enum PinType {
    UNPINNED,
    PINNED,
    ALWAYS,
    NEVER
  }

  public abstract class ScrollerChild : Ctk.Actor
  {
    construct
    {
    }

    public Gdk.Pixbuf icon {get; set;}
    public PinType pin_type;
    public float position {get; set;}
    public bool running {get; set;}
    public bool active {get; set;}
    public bool needs_attention {get; set;}
    public bool activating {get; set;}
    public float rotation {get; set;}
    public ScrollerChildController controller; // this sucks. shouldn't be here, can't help it.

    public abstract void force_rotation_jump (float degrees);

    public string to_string ()
    {
      return "A scroller child; running: %s, active: %s, position: %f, opacity %f".printf (
                (running) ? "yes" : "no",
                (active) ? "yes" : "no",
                position,
                opacity);
    }
  }
}
