/* -*- Mode: vala; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
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
 * Authored by Gordon Allott <gord.allott@canonical.com>
 *
 */
using Unity.Quicklauncher;
namespace Unity.Widgets
{

  public class ScrollerChild : Object
  {
    /* just a container for our children, we use an object basically just for
     * memory management. if the gobjectifying becomes an issue it should
     * be trivial to revert it to a struct with manual memory management
     * (ie: use no gobject features)
     */

    public float width = 0.0f;
    public float height = 0.0f;

    public float position = 0.0f;

    public Clutter.Actor child;
    public Clutter.ActorBox box;

    private bool _is_hidden = false;
    public bool is_hidden {
      get { return this._is_hidden; }
      set {
        if (hide_anim is Clutter.Animation)
          hide_anim.completed ();

        if (_is_hidden && value == false)
          this.hide_anim = this.child.animate (
            Clutter.AnimationMode.EASE_OUT_QUAD,
            200, "opacity", 255);

        if (!_is_hidden && value == true)
          this.hide_anim = this.child.animate (
            Clutter.AnimationMode.EASE_OUT_QUAD,
            200, "opacity", 64);
        this._is_hidden = value;
      }
    }

    private Clutter.Animation hide_anim;

    public ScrollerChild ()
    {
    }
  }

  public class Scroller : Ctk.Actor, Clutter.Container
  {
    private float _drag_pos = -0.0f;
    public float drag_pos {
      get { return this._drag_pos; }
      set {
        this._drag_pos = value;
        this.queue_relayout ();
      }
    }
    private bool _is_dragging = false;
    private bool is_dragging {
        get { return this._is_dragging; }
        set
          {
            if (!this.is_dragging)
              {
                this.fling_timeline.stop ();
                if (this.scroll_anim is Clutter.Animation)
                  {
                    if (this.scroll_anim.has_property ("drag_pos"))
                      {
                        this.scroll_anim.unbind_property ("drag_pos");
                      }
                  }
              }
            foreach (ScrollerChild child in children)
              {
                child.child.set_reactive (!value);
              }
            this._is_dragging = value;
          }
    }
    private const float friction = 0.9f;
    private float last_drag_pos = 0.0f;
    private float fling_velocity = 0.0f;
    private float previous_y = -1000000000.0f;
    private float fling_target = 0.0f;

    private Clutter.Texture? bgtex;
    private Clutter.Texture? gradient;
    private Clutter.Rectangle edge;

    private int _spacing;
    public int spacing {
      get { return _spacing; }
      set { queue_relayout (); _spacing = value; }
    }
    private Ctk.Orientation orientation = Ctk.Orientation.VERTICAL;

    private Gee.ArrayList<ScrollerChild> children;
    private float total_child_height;

    private Gee.HashMap<Clutter.Actor, Clutter.Animation> fadeout_stack;
    private Gee.HashMap<Clutter.Actor, Clutter.Animation> fadein_stack;
    private Gee.HashMap<Clutter.Actor, Clutter.Animation> anim_stack;
    private Clutter.Animation scroll_anim;

    private float hot_height = 0.0f;
    private float hot_start = 0.0f;

    private Clutter.Timeline fling_timeline;
    private bool button_down = false;

    public Scroller (Ctk.Orientation orientation, int spacing)
    {
      this.orientation = orientation;
      this.spacing = spacing;
      children = new Gee.ArrayList<ScrollerChild> ();
      fadeout_stack = new Gee.HashMap<Clutter.Actor, Clutter.Animation> ();
      fadein_stack = new Gee.HashMap<Clutter.Actor, Clutter.Animation> ();
      anim_stack = new Gee.HashMap<Clutter.Actor, Clutter.Animation> ();
    }

    construct {
        try
          {
            bgtex = new Clutter.Texture.from_file (
                                        Unity.PKGDATADIR + "/honeycomb.png");
          }
        catch (Error e)
          {
            error ("Unable to load texture '%s': %s",
                     Unity.PKGDATADIR + "/honeycomb.png",
                     e.message);
          }
      try
        {
          gradient = new Clutter.Texture.from_file (
                                           Unity.PKGDATADIR + "/gradient.png");
        }
      catch (Error e)
        {
          error ("Unable to load texture: '%s': %s",
                   Unity.PKGDATADIR + "/gradient.png",
                   e.message);
        }

      var mypadding = this.padding;

      mypadding.left = 0.0f;
      mypadding.right = 0.0f;
      mypadding.top = 6.0f;
      mypadding.bottom = 6.0f;

      this.padding = mypadding;

      bgtex.set_repeat (true, true);
      bgtex.set_opacity (0xE0);

      gradient.set_repeat (false, true);
      gradient.set_opacity (0x30);

      bgtex.set_parent (this);
      bgtex.queue_relayout ();

      gradient.set_parent (this);
      gradient.queue_relayout ();

      var edge_color = Clutter.Color ()
      {
        red = 0xff,
        green = 0xff,
        blue = 0xff,
        alpha = 0x80
      };
      edge = new Clutter.Rectangle.with_color (edge_color);
      edge.set_parent (this);

      // set a timeline for our fling animation
      this.fling_timeline = new Clutter.Timeline (1000);
      this.fling_timeline.loop = true;
      this.fling_timeline.new_frame.connect (this.on_fling_frame);
      set_reactive (true);

      this.scroll_event.connect (this.on_scroll_event);
      this.button_press_event.connect (this.on_button_click_event);
      this.button_release_event.connect (this.on_button_release_event);
      this.motion_event.connect (this.on_motion_event);

      this.drag_pos = 0.0f;
    }

    private void on_request_attention (LauncherView view)
    {
      /* when the app requests attention we need to scroll to it */
      // check to make sure we can actually scroll
      if (this.total_child_height > this.hot_height)
        return;

      // find the app in our list
      Clutter.Actor actor = view as Clutter.Actor;
      foreach (ScrollerChild container in this.children)
      {
        if (container.child == actor)
        {
          float scroll_px;
          scroll_px = container.box.y2 - hot_height - hot_start;

          if (scroll_anim is Clutter.Animation)
            scroll_anim.completed ();

          scroll_anim = animate (Clutter.AnimationMode.EASE_OUT_QUAD,
                                 200, "drag_pos",
                                 scroll_px);
          return;
        }
      }
    }

    private uint stored_delta = 0;
    private void on_fling_frame (Clutter.Timeline timeline, int msecs)
    {

      // the jist here is that we are doing a *real* per frame fling so we need
      // to head towards a given goal, bouncing around on the way

      float difference = this.drag_pos - this.fling_target;
      this.fling_velocity += difference * 0.005f;


      if (difference < 1.0f && difference > -1.0f)
        {
          // we have arrived at our destination.
          timeline.stop ();
          this.last_drag_pos = this.fling_target;
          this.drag_pos = this.fling_target;
          this.stored_delta = 0;
          this.previous_y = this.drag_pos;
        }

      /* this code is slightly alkward, physics engines really need to tick
       * at a constant framerate, even simple ones like this, but we can't
       * do that nicely in clutter so we have to "catch-up" and "slow down"
       * depending on how close to our target framerate (60fps) we are
       */

      uint delta = timeline.get_delta ();
      delta += this.stored_delta;
      if (delta <= 16)
        {
          this.stored_delta = delta;
          return;
        }

      float d = 0.0f;
      while (delta > 16)
        {
          delta -= 16;
          this.fling_velocity *= this.friction;
          d -= this.fling_velocity;
        }
      this.drag_pos += d;

      this.stored_delta = delta;
    }

    private void calculate_anchor (out int iterations, out float position)
    {
      float d = 0.0f;
      float v = fling_velocity;
      iterations = 0;
      while (v >= 1.0f || v <= -1.0)
        {
          iterations += 1;
          v *= this.friction;
          d -= v;
        }
      position = (float)this.drag_pos + d;
      return;
    }

    private float get_next_neg_position (float target_pos)
    {
      /* returns the scroll to position of the next neg item */
      /* *sigh* gee has no reverse() method for its container so we can't
       * easily reverse the itteration, have to just do it manually
       */
      for (var i = this.children.size - 1; i >= 0; i--)
      {
        ScrollerChild container = this.children.get(i);
        Clutter.ActorBox box = container.box;
        if (box.y1 == 0.0 && box.y2 == 0.0) continue;
        if (box.y1 < hot_start)
        {
          /* we have a container lower than the "hotarea" */
          float scroll_px = box.y1 + target_pos - hot_start;
          return (float)scroll_px - this.padding.top;
        }
      }
      return 0.0f;
    }

    private bool on_button_click_event (Clutter.Event event)
    {
      if (event.button.button != 1)
        {
           return false;
        }

      if (this.get_animation() is Clutter.Animation)
        {
          this.get_animation().completed ();
        }
      this.button_down = true;
      Clutter.ButtonEvent buttonevent = event.button;
      this.previous_y = buttonevent.y;
      this.last_drag_pos = (float)this.drag_pos;
      return true;
    }

    private bool on_button_release_event (Clutter.Event event)
    {
      if (event.button.button != 1)
        {
           return false;
        }
      this.button_down = false;

      if (this.is_dragging)
        {
          this.is_dragging = false;
          Clutter.ungrab_pointer ();
        }

      int iters = 0;
      float position = 0.0f;

      calculate_anchor (out iters, out position);

      if (position < 0.0 || position > this.total_child_height - this.height)
        {
          /* because we are flinging outside of our target area we have to
           * do a "real" fling, per frame calculated and then swing back
           * to a nice position
           */
          // because we are out of bounds with our target position we clamp
          // the value to the top item or the bottom item#
          if (position < 0.0 || this.total_child_height < this.height)
            {
              this.fling_target = 0.0f;
            }
          else
            {
              this.fling_target = this.total_child_height - this.height + 32;
            }
          this.fling_timeline.start ();
        }
      else
        {
          /* because we are flinging inside our target area then we can do
           * a fake fling and just use clutters animation system, should
           * hopefully give a nicer feel
           */
          if (this.scroll_anim is Clutter.Animation)
            {
              this.scroll_anim.completed ();
            }

          if (iters > 0)
            {

              position = get_next_neg_position (position);
              this.scroll_anim = this.animate (
                                    Clutter.AnimationMode.EASE_OUT_QUAD,
                                    16 * iters, "drag_pos", position);
            }
        }

      return true;
    }

    private bool on_motion_event (Clutter.Event event)
    {
      if (this.button_down)
        {
          this.is_dragging = true;

          Clutter.grab_pointer (this);

          /* Disable any animations on the children */
          Clutter.Event e = { 0 };
          e.type = Clutter.EventType.LEAVE;
          e.crossing.time = event.motion.time;
          e.crossing.flags = event.motion.flags;
          e.crossing.stage = event.motion.stage;
          e.crossing.x = event.motion.x;
          e.crossing.y = event.motion.y;

          foreach (ScrollerChild container in this.children)
            {
              Clutter.Actor child = container.child;

              if (child is Clutter.Actor)
                {
                  e.crossing.source = child;
                  child.do_event (e, false);
                }
            }
        }
      if (this.is_dragging == false)
        {
          return true;
        }

      Clutter.MotionEvent motionevent = event.motion;
      float vel_y = 0.0f;

      if (previous_y <= -1000000000.0)
      {
        vel_y = 0.0f;
      }
      else
      {
        vel_y = motionevent.y - previous_y;
      }
      previous_y = motionevent.y;
      this.drag_pos -= vel_y;
      this.fling_velocity = vel_y;

      return true;
    }

    private bool on_scroll_event (Clutter.Event event)
    {
      Clutter.ScrollEvent scrollevent = event.scroll;
      if (scrollevent.direction == Clutter.ScrollDirection.UP)
      {
        this.fling_velocity = 16.0f;
        this.fling_timeline.start ();
      }
      else if (scrollevent.direction == Clutter.ScrollDirection.DOWN)
      {
        this.fling_velocity = -16.0f;
        this.fling_timeline.start ();
      }
      return true;
    }

    /*
     * Clutter methods
     */
    public override void get_preferred_width (float for_height,
                                     out float minimum_width,
                                     out float natural_width)
    {
      minimum_width = 0;
      natural_width = 0;

      // if we are vertical, just figure out the width of the widest
      // child
      if (orientation == Ctk.Orientation.VERTICAL)
      {
        float pmin_width = 0.0f;
        float pnat_width = 0.0f;

        foreach (ScrollerChild childcontainer in this.children)
        {
          Clutter.Actor child = childcontainer.child;
          float cmin_width = 0.0f;
          float cnat_width = 0.0f;

          child.get_preferred_width (for_height,
                                     out cmin_width,
                                     out cnat_width);

          pmin_width = pmin_width.max(pmin_width, cmin_width);
          pnat_width = pnat_width.max(pnat_width, cnat_width);

        }

        pmin_width += padding.left + padding.right;
        pnat_width += padding.left + padding.right;

        minimum_width = pmin_width;
        natural_width = pnat_width;

        return;
      }

      // we have no support for horizontal yet
      error ("no support for horizontal orientation yet");

    }

    public override void get_preferred_height (float for_width,
                                      out float minimum_height,
                                      out float natural_height)
    {
      minimum_height = 0.0f;
      natural_height = 0.0f;

      float cnat_height = 0.0f;
      float cmin_height = 0.0f;
      float all_child_height = 0.0f;

      if (orientation == Ctk.Orientation.VERTICAL)
      {
        foreach (ScrollerChild childcontainer in this.children)
        {
          Clutter.Actor child = childcontainer.child;
          cnat_height = 0.0f;
          cmin_height = 0.0f;
          child.get_preferred_height (for_width,
                                      out cmin_height,
                                      out cnat_height);

          all_child_height += cnat_height;
        }

        minimum_height = all_child_height + padding.top + padding.bottom;
        natural_height = all_child_height + padding.top + padding.bottom;
        return;
      }
      error ("Does not support Horizontal yet");
    }

    public override void allocate (Clutter.ActorBox box,
                                   Clutter.AllocationFlags flags)
    {
      base.allocate (box, flags);

      this.height = box.y2 - box.y1;
      this.total_child_height = 0.0f;
      float x = padding.left;
      float y = padding.top;
      float hot_negative = 0;
      float hot_positive = box.get_height ();// + this.padding.top - this.padding.bottom;
      this.hot_start = hot_negative;
      Clutter.ActorBox child_box;

      y = hot_start - (float)this.drag_pos;
      hot_height = hot_positive - hot_negative;

      /* itterate though each child and position correctly
       * also whist we are here we check to see if the child is outside of our
       * "hot" area, if so, mark it as unresponsive and fade it out
       */
      foreach (ScrollerChild childcontainer in this.children)
      {
        Clutter.Actor child = childcontainer.child;
        float min_height, natural_height;
        child.get_allocation_box (out child_box);
        child.get_preferred_height (box.get_width (), out min_height, out natural_height);
        if (orientation == Ctk.Orientation.VERTICAL)
        {

          child_box.x1 = x;
          child_box.x2 = x + child.width + padding.right;
          child_box.y1 = y;
          child_box.y2 = y + min_height;

          y += child.height + spacing;
          this.total_child_height += child.height + spacing;
        }
        else
        {
          error ("Does not support Horizontal yet");
        }

        childcontainer.box = child_box;
        child.allocate (child_box, flags);

        // if the child is outside our hot area, we fade it out and make it
        // unresponsive
        if ((child_box.y1 < hot_negative) || (child_box.y2 > hot_positive))
          {
            if (!childcontainer.is_hidden)
              {
                childcontainer.is_hidden = true;
                child.set_reactive (false);
              }

            // we need to set a clip on each actor
            if (child_box.y1 < hot_negative)
              {
                var yclip = hot_negative - child_box.y1;
                child.set_clip (0, yclip,
                                child_box.get_width (),
                                child_box.get_height () - yclip);
              }
            else if (child_box.y2 > hot_positive)
              {
                var yclip = child_box.y2 - hot_positive;
                child.set_clip (0, 0,
                                child_box.get_width (),
                                child_box.get_height () - yclip);
              }

          }
        else
          {
            if (childcontainer.is_hidden)
              {
                childcontainer.is_hidden = false;
                child.set_reactive (true);
              }
              child.set_clip (0, 0,
                              child_box.get_width (), child_box.get_height ());
          }


      }

      /* also allocate our background graphics */
      bgtex.get_allocation_box (out child_box);
      child_box.y1 = box.y1 - (float)drag_pos - box.get_height () * 2;
      child_box.y2 = box.y2 - (float)drag_pos + box.get_height () * 2;
      child_box.x1 = box.x1;
      child_box.x2 = box.x2;
      bgtex.allocate (child_box, flags);
      bgtex.set_clip (box.x1, drag_pos + box.get_height () * 2,
                      box.get_width (), box.get_height ());

      gradient.width = box.get_width();
      gradient.allocate (box, flags);

      edge.width = 1;
      edge.height = box.get_height ();
      Clutter.ActorBox edge_box;
      edge.get_allocation_box (out edge_box);
      edge_box.y1 = box.y1;
      edge_box.y2 = box.y2;
      edge_box.x2 = box.x2+1;
      edge_box.x1 = box.x2;
      edge.allocate (edge_box, flags);
    }

    public override void pick (Clutter.Color color)
    {
      base.pick (color);
      foreach (ScrollerChild childcontainer in this.children)
      {
        Clutter.Actor child = childcontainer.child;
        child.paint ();
        child.pick (color);
      }
    }


    public override void paint ()
    {
      bgtex.paint ();
      gradient.paint ();

      foreach (ScrollerChild childcontainer in this.children)
      {
        Clutter.Actor child = childcontainer.child;
        if ((child.flags & Clutter.ActorFlags.VISIBLE) != 0)
          child.paint ();
      }

      edge.paint ();
    }


    public void add (Clutter.Actor actor)
    {
      this.add_actor (actor);
    }

    public void add_actor (Clutter.Actor actor)
      requires (this.get_parent () != null)
    {
      var container = new ScrollerChild ();
      container.child = actor;
      this.children.add (container);
      actor.set_parent (this);

      /* if we have an LauncherView we need to tie it to our attention
       * grabber
       */
      if (actor is LauncherView)
      {
        LauncherView view = actor as LauncherView;
        view.request_attention.connect (on_request_attention);
      }

      /* set a clip on the actor */
      actor.set_clip (0, -200, 58, 400);

      this.queue_relayout ();
      this.actor_added (actor);
    }

    public void remove (Clutter.Actor actor)
    {
      this.remove_actor (actor);
    }

    public void remove_actor (Clutter.Actor actor)
    {
      ScrollerChild found_container = null;
      foreach (ScrollerChild container in this.children) {
        if (container.child == actor)
        {
          found_container = container;
          break;
        }
      }
      if (found_container is ScrollerChild)
      {
        found_container.child = null;
        this.children.remove (found_container);
        actor.unparent ();

        this.queue_relayout ();
        this.actor_removed (actor);
        actor.remove_clip ();
      }
    }

    public void @foreach (Clutter.Callback callback, void* userdata)
    {
      foreach (ScrollerChild childcontainer in this.children)
      {
        Clutter.Actor child = childcontainer.child;
        callback (child, null);
      }
    }

    public void foreach_with_internals (Clutter.Callback callback,
                                        void* userdata)
    {
      callback (bgtex, null);
      callback (gradient, null);
      callback (edge, null);
      foreach (ScrollerChild childcontainer in this.children)
      {
        Clutter.Actor child = childcontainer.child;
        callback (child, null);
      }
    }


    /* empty interface methods */
    public void create_child_meta (Clutter.Actor actor)
    {
    }

    public void destroy_child_meta (Clutter.Actor actor)
    {
    }

    public new void lower (Clutter.Actor actor, Clutter.Actor sibling)
    {
    }

    public new void raise (Clutter.Actor actor, Clutter.Actor sibling)
    {
    }
    /* has to return something, implimentation does not have ? so we can't
     * return null without a warning =\
     */
    public Clutter.ChildMeta? get_child_meta (Clutter.Actor actor)
    {
      return null;
    }

    public void sort_depth_order ()
    {
    }
  }
}
