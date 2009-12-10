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
 * Authored by Mirco "MacSlow" Müller <mirco.mueller@canonical.com>
 *
 */

namespace Unity.Places
{
  public class View : Ctk.Box
  {
    private Unity.Places.Bar.View     bar_view;
    private Unity.Places.Default.View default_view;

    public override void allocate (Clutter.ActorBox        box, 
                                   Clutter.AllocationFlags flags)
    {
      base.allocate (box, flags);
      Clutter.ActorBox child_box = { 0.0f, 0.0f, 0.0f, 0.0f };

      child_box.x1 = this.padding.left;
      child_box.x2 = box.x2 - box.x1 - this.padding.left - this.padding.right;
      child_box.y1 = this.padding.top;
      child_box.y2 = child_box.y1 + 64;

      this.bar_view.allocate (child_box, flags);
      child_box.y1 = this.padding.top + 64;
      child_box.y2 = box.y2 - box.y1 - this.padding.top -this.padding.bottom - 64;

      this.default_view.allocate (child_box, flags);
    }

    public View ()
    {
      this.orientation  = Ctk.Orientation.VERTICAL;
      this.bar_view     = new Unity.Places.Bar.View ();
      this.default_view = new Unity.Places.Default.View ();
      this.add_actor (this.bar_view);
      this.add_actor (this.default_view);

      Ctk.Padding padding = { 0.0f, 0.0f, 0.0f, 12.0f };
      this.set_padding (padding);     
    }

    public void set_size_and_position (int bar_x,
                                       int bar_y,
                                       int bar_w,
                                       int bar_h,
                                       int def_view_x,
                                       int def_view_y,
                                       int def_view_w,
                                       int def_view_h)
    {
      this.bar_view.x      = bar_x;
      this.bar_view.y      = bar_y;
      this.bar_view.width  = bar_w;
      this.bar_view.height = bar_h;

      this.default_view.x      = def_view_x;
      this.default_view.y      = def_view_y;
      this.default_view.width  = def_view_w;
      this.default_view.height = def_view_h;
    }

    construct
    {
    }
  }
}
