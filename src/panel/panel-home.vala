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

namespace Unity.Panel
{
  public class HomeButton : Ctk.Bin
  {
    public Shell shell { get; construct; }
    public ThemeImage image;

    public signal void clicked (uint32 time_);

    public HomeButton (Shell shell)
    {
      Object (reactive:true,
              shell:shell);
    }

    construct
    {
      this.image = new ThemeImage ("distributor-logo");
      this.add_actor (this.image);
      this.image.show ();

      this.button_release_event.connect (this.on_button_release);
    }

    private override void allocate (Clutter.ActorBox        box,
                                    Clutter.AllocationFlags flags)
    {
      float cwidth, cheight;
      Ctk.Padding pad = { 0 };

      this.image.get_preferred_size (out cwidth, out cheight,
                                     out cwidth, out cheight);

      /* Just in case the loading is slow */
      if (width < 1)
        cwidth = 19;
      if (height < 1)
        cheight = 19;

      pad.left = (box.x2 - box.x1 - cwidth) / 2.0f;
      pad.right = pad.left;
      pad.top = (box.y2 - box.y1 - cheight) / 2.0f;
      pad.bottom = pad.top;

      this.padding = pad;

      base.allocate (box, flags);
    }

    private bool on_button_release (Clutter.Event event)
    {
      this.clicked (event.button.time);

      return true;
    }
  }
}
