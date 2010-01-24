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
  public class View : Ctk.Actor
  {
    private Shell _shell;
    public  Shell shell {
      get { return _shell; }
      set { _shell = value; }
    }

    private Tray.View tray;

    private Clutter.Rectangle rect;

    public View (Shell shell)
    {
      Object (shell:shell);
      this.tray.manage_stage (this.shell.get_stage ());
    }

    construct
    {
      START_FUNCTION ();

      Clutter.Color color = { 0, 0, 0, 180 };

      this.rect = new Clutter.Rectangle.with_color (color);
      this.rect.set_parent (this);
      this.rect.show ();

      this.tray = new Tray.View ();
      this.tray.set_parent (this);
      this.tray.show ();
      //this.tray.manage_stage (this._shell.get_stage ());

      END_FUNCTION ();
    }

    private override void allocate (Clutter.ActorBox        box,
                                    Clutter.AllocationFlags flags)
    {
      Clutter.ActorBox child_box = { 0, 0, box.x2 - box.x1, box.y2 - box.y1 };
      this.rect.allocate (child_box, flags);

      child_box.x1 = 100;
      child_box.x2 = 200;
      this.tray.allocate (child_box, flags);
    }

    private override void paint ()
    {
      base.paint ();
      this.rect.paint ();
      this.tray.paint ();
    }

    private override void map ()
    {
      base.map ();
      this.rect.map ();
      this.tray.map ();
    }

    private override void unmap ()
    {
      base.unmap ();
      this.rect.unmap ();
      this.tray.unmap ();
    }
  }
}
