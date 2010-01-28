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
    public Shell shell { get; construct; }

    private Bar.View       bar_view;
    private Clutter.Actor? content_view;
	  private SearchField.View search_view;

    public View (Model model, Shell shell)
    {
      Ctk.Padding padding = { 0.0f, 0.0f, 0.0f, 68.0f };

      Object (model:model, shell:shell);

      this.orientation  = Ctk.Orientation.VERTICAL;

      this.bar_view = new Bar.View (this.model, this.shell);
      this.bar_view.padding = padding;
      this.add_actor (this.bar_view);

      this.search_view = new SearchField.View ();
      this.add_actor (this.search_view);
      
    }

    public override void allocate (Clutter.ActorBox        box,
                                   Clutter.AllocationFlags flags)
    {
      Clutter.ActorBox child_box = { 0.0f, 0.0f, 0.0f, 0.0f };

      child_box.x1 = 0;
      child_box.x2 = box.x2 - box.x1;
      child_box.y1 = 0;
      child_box.y2 = 62;

      this.bar_view.allocate (child_box, flags);

      child_box.x1 = 58;
      child_box.x2 = box.x2 - box.x1;
      child_box.y1 = box.y1 + 62;
      child_box.y2 = box.y2 - box.y1;

      if (this.content_view is Clutter.Actor)
        {
          this.content_view.allocate (child_box, flags);
        }

      child_box.x1 = child_box.x2 - this.shell.get_indicators_width() + 8;
      child_box.x2 = child_box.x1 + this.shell.get_indicators_width() - 8;
      child_box.y1 = 32;
      child_box.y2 = child_box.y1 + 22;
      this.search_view.allocate (child_box, flags);
    }

    /* Public methods */
    public void set_content_view (Clutter.Actor actor)
    {
      if (this.content_view is Clutter.Actor)
      {
        this.content_view.animate (Clutter.AnimationMode.EASE_OUT_QUAD, 300,
                                   "opacity", 0,
                                   "signal::completed", on_fade_out_done,
                                   this.content_view);
      }

      this.content_view = actor;
      this.add_actor (this.content_view);
      this.content_view.show ();

      actor.opacity = 0;
      actor.animate (Clutter.AnimationMode.EASE_IN_QUAD, 300,
                     "opacity", 255);
    }

    static void on_fade_out_done (Clutter.Animation anim, Clutter.Actor actor)
    {
      actor.destroy ();
    }
  }
}

