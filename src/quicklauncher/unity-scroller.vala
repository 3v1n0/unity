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
    private double _drag_pos = -0.0;
    public double drag_pos {
      get { return this._drag_pos; }
      set {
        this._drag_pos = value;
        this.queue_relayout ();
      }
    }
    private bool is_dragging = false;
    private float last_drag_pos = 0.0f;
    private float fling_velocity = 0.0f;

    private float previous_y = -1000000000.0f;

    private Clutter.Texture bgtex;
    private Clutter.Texture gradient;
    private Clutter.Rectangle edge;

    private int _spacing;
    public int spacing {
      get { return _spacing; }
      set { queue_relayout (); _spacing = value; }
    }
    private Ctk.Orientation orientation = Ctk.Orientation.VERTICAL;

    private Gee.ArrayList<ScrollerChild> children;
    private double total_child_height;

    private Gee.HashMap<Clutter.Actor, Clutter.Animation> fadeout_stack;
    private Gee.HashMap<Clutter.Actor, Clutter.Animation> fadein_stack;
    private Gee.HashMap<Clutter.Actor, Clutter.Animation> anim_stack;
    private Clutter.Animation scroll_anim;

    private double hot_height = 0.0;
    private float hot_start = 0.0f;

    private Clutter.Timeline fling_timeline;

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
      bgtex = new Clutter.Texture.from_file (
        Unity.PKGDATADIR + "/honeycomb.png"
        );
      assert (bgtex is Clutter.Texture);

      gradient = new Clutter.Texture.from_file (
        Unity.PKGDATADIR + "/gradient.png"
        );
      assert (gradient is Clutter.Texture);

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
      this.leave_event.connect (this.on_leave_event);

      this.drag_pos = 0.0f;
    }

    private void on_request_attention (Unity.Quicklauncher.ApplicationView app)
    {
      /* when the app requests attention we need to scroll to it */
      // check to make sure we can actually scroll
      if (this.total_child_height > this.hot_height)
        return;

      // find the app in our list
      Clutter.Actor actor = app as Clutter.Actor;
      foreach (ScrollerChild container in this.children) 
      {
        if (container.child == actor)
        {
          double scroll_px = container.box.y2 + drag_pos - hot_height - hot_start;

          if (scroll_anim is Clutter.Animation)
            scroll_anim.completed ();

          scroll_anim = animate (Clutter.AnimationMode.EASE_OUT_QUAD, 
                                 200, "drag_pos", 
                                 scroll_px);
          return;
        }
      }     
    }

    private float get_next_pos_position () 
    {
      /* returns the scroll to position of the next pos item */
      double hot_end = hot_start + hot_height - 1;
      foreach (ScrollerChild container in this.children)
      {
        Clutter.ActorBox box = container.box;
        if (container == this.children.last())
          return 1.0f;
        
        if (box.y2 > hot_end)
        {
          /* we have a container greater than the 'hotarea' 
           * we should scroll to that
           */
          double scroll_px = box.y2 + drag_pos - hot_height - hot_start + spacing;
          return (float)scroll_px;
        } 
      }
      return 1.0f;   
    }

    private float get_next_neg_position ()
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
          double scroll_px = box.y1 + drag_pos - hot_start + 5;
          return (float)scroll_px;
        }
      }
      return 0.0f;
    }

    private uint fling_accumulated_frames = 0;
    private void on_fling_frame (Clutter.Timeline timeline, int msecs)
    {
      if (fling_velocity < 1.0f && fling_velocity > -1.0f) 
        {
          timeline.pause ();
        }

      /* this code is slightly alkward, physics engines really need to tick
       * at a constant framerate, even simple ones like this, but we can't
       * do that nicely in clutter so we have to "catch-up" and "slow down"
       * depending on how close to our target framerate (60fps) we are 
       */

      uint delta = timeline.get_delta ();
      if (delta < 1) //should never be < 1 but lets check anyway
        {
          return;
        }

      fling_accumulated_frames = delta;
      while (fling_accumulated_frames > 16)
        {
          // we missed a frame
          fling_accumulated_frames -= 16;
          fling_velocity *= 0.90f;
          
          // could be sped up by avoiding the drag_pos setter here
          this.drag_pos -= fling_velocity;
        }
      if (fling_accumulated_frames > 0)
        {
          float vel_mod = (float)fling_accumulated_frames / 16.0f;
          fling_velocity *= 1.0f - (0.1f * vel_mod);
          this.drag_pos -= fling_velocity;
          fling_accumulated_frames = 0;
        }
    }

    /**
     * scrolls a single item negatively (false) or positively (true)
     */
    private void scroll_single_item (bool direction) 
    {
      //if (scroll_anim is Clutter.Animation)
      //  scroll_anim.completed ();

      var next_pos = 0.0f;
      if (!direction)
        next_pos = get_next_neg_position ();
      else
        next_pos = get_next_pos_position ();

      scroll_anim = animate (Clutter.AnimationMode.EASE_OUT_QUAD, 200, 
                             "drag_pos", next_pos);
    }

    private bool on_button_click_event (Clutter.Event event)
    {
      // FIXME do button check on event
      debug ("got click event");
      foreach (ScrollerChild childcontainer in this.children)
      {
        Clutter.Actor child = childcontainer.child;
        child.set_reactive (false);
      }

      Clutter.ButtonEvent buttonevent = event.button;
      this.is_dragging = true;
      this.previous_y = buttonevent.y;
      this.last_drag_pos = (float)this.drag_pos;
      return true;
    }

    private bool on_button_release_event (Clutter.Event event)
    {
      // FIXME do a button check on event
      debug ("got release event");
      this.is_dragging = false;
      //scroll_single_item (true);

      this.fling_timeline.start ();

      return true;
    }

    private bool on_motion_event (Clutter.Event event)
    {
      if (this.is_dragging == false)
        return true;
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

    private bool on_leave_event (Clutter.Event event)
    {
      debug ("leaving");
      if (this.is_dragging)
      {
        this.is_dragging = false;
        scroll_anim = animate (Clutter.AnimationMode.EASE_OUT_QUAD, 
                               600, "drag_pos", this.last_drag_pos);
      }
      return true;
    }

    private bool on_scroll_event (Clutter.Event event)
    {
      Clutter.ScrollEvent scrollevent = event.scroll;
      if (scrollevent.direction == Clutter.ScrollDirection.UP)
      {
        scroll_single_item (false);
      }
      else if (scrollevent.direction == Clutter.ScrollDirection.DOWN)
      {
        scroll_single_item (true);
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

      total_child_height = 0.0f;
      foreach (ScrollerChild childcontainer in this.children)
      {
        Clutter.Actor child = childcontainer.child;
        float min_width = 0.0f;
        float min_height = 0.0f;
        float nat_width = 0.0f;
        float nat_height = 0.0f;

        child.get_preferred_width(box.y2 - box.y1, 
                                  out min_width, out nat_width);

        child.get_preferred_height(box.x2 - box.x1,
                                   out min_height, out nat_height);
        child.width = min_width;
        child.height = min_height;
        total_child_height += child.height + spacing;
      }

      // position the actors

      float x = padding.left;
      float y = padding.top;
      Clutter.ActorBox child_box;
      
      float hot_negative = padding.top;
      this.hot_start = hot_negative;
      float hot_positive = box.get_height () + this.padding.top - this.padding.bottom;

      y = hot_start - (float)this.drag_pos;
      hot_height = hot_positive - hot_negative;

      /* itterate though each child and position correctly
       * also whist we are here we check to see if the child is outside of our
       * "hot" area, if so, mark it as unresponsive and fade it out
       */
      foreach (ScrollerChild childcontainer in this.children)
      {
        Clutter.Actor child = childcontainer.child;
        child.get_allocation_box (out child_box);
        if (orientation == Ctk.Orientation.VERTICAL)
        {
          
          child_box.x1 = x;
          child_box.x2 = x + child.width + padding.right;
          child_box.y1 = y;
          child_box.y2 = y + child.height;
          
          y += child.height + spacing;
        }
        else 
        {
          error ("Does not support Horizontal yet");
        }
        
        childcontainer.box = child_box;
        child.allocate (child_box, flags);
      }

      /* also allocate our background graphics */
      bgtex.get_allocation_box (out child_box);
      child_box.y1 = box.y1 - (float)drag_pos - box.get_height ();
      child_box.y2 = box.y2 - (float)drag_pos + box.get_height ();
      child_box.x1 = box.x1;
      child_box.x2 = box.x2;
      bgtex.allocate (child_box, flags);
      
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
      
      handle_overflow ();
    }

    private void handle_overflow ()
    {

      float hot_negative = this.padding.top;
      float hot_positive = this.height - this.padding.bottom;

      foreach (ScrollerChild childcontainer in this.children)
      {
        var child = childcontainer.child;
        var child_box = childcontainer.box;

        if (((child_box.y1 < hot_negative) || (child_box.y2 > hot_positive)))
        {
          if (!childcontainer.is_hidden)
          {
            childcontainer.is_hidden = true;
            child.set_reactive (false);
          }
        }
        else
        {
          if (childcontainer.is_hidden)
          {
            childcontainer.is_hidden = false;
            child.set_reactive (true);
          }
        }
      }
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
    }

    public void add_actor (Clutter.Actor actor)
      requires (this.get_parent () != null)
    {
      var container = new ScrollerChild ();
      container.child = actor;
      this.children.add (container);
      actor.set_parent (this);

      /* if we have an ApplicationView we need to tie it to our attention 

       * grabber
       */
      if (actor is Unity.Quicklauncher.ApplicationView)
      {
        Unity.Quicklauncher.ApplicationView app = actor as Unity.Quicklauncher.ApplicationView;
        app.request_attention.connect (on_request_attention);
      }

      /* set a clip on the actor */
      actor.set_clip (0, -200, 56, 400);

      this.queue_relayout ();
      this.actor_added (actor);
    }

    public void remove (Clutter.Actor actor)
    {

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
