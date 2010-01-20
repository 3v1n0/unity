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
 *             Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */

namespace Unity.Places
{
  public class View : Ctk.Box
  {
    public Model model { get; construct; }

    private Bar.View     bar_view;
    private Default.View default_view;

    public View (Model model)
    {
      Ctk.Padding bpadding = { 0.0f, 0.0f, 0.0f, 68.0f };

      Object (model:model);

      this.orientation  = Ctk.Orientation.VERTICAL;

      this.bar_view = new Bar.View (this.model);
      this.bar_view.padding = bpadding;

      this.default_view = new Default.View ();
      this.add_actor (this.bar_view);
      this.add_actor (this.default_view);

      Ctk.Padding padding = { 0.0f, 0.0f, 0.0f, 12.0f };
      this.set_padding (padding);
    }

    /* These parameters are temporary until we get the right metrics for
     * the places bar
     */
    public override void allocate (Clutter.ActorBox        box,
                                   Clutter.AllocationFlags flags)
    {
      base.allocate (box, flags);
      Clutter.ActorBox child_box = { 0.0f, 0.0f, 0.0f, 0.0f };

      child_box.x1 = 0;
      child_box.x2 = box.x2 - box.x1;
      child_box.y1 = 0;
      child_box.y2 = 62;

      this.bar_view.allocate (child_box, flags);

      child_box.x1 = box.x1 + 58 /* HARDCODED: width of quicklauncher */;
      child_box.x2 = box.x2;
      child_box.y1 = box.y1 + 55 /* HARDCODED: height of Places bar */;
      child_box.y2 = box.y2;

      this.default_view.allocate (child_box, flags);
    }
  }
}

