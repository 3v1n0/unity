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
  private struct ScrollerItemInfo
  {
    float position;
    float height;
    float width;
  }

  public class Scroller : Ctk.Actor, Clutter.Container
  {
    
    public double scroll_pos;
    private double scroll_value_as_px {
      get 
      { 
        var pos = scroll_pos * (this.total_child_height - this.height);
        return pos.max(0.0, pos);
      }
    }

    private Clutter.Texture bgtex;
    private Clutter.Texture gradient;

    // our two action images, positive and negative
    private Ctk.Image action_negative;
    private Ctk.Image action_positive;

    private int _spacing;
    public int spacing {
      get { return _spacing; }
      set { queue_relayout (); _spacing = value; }
    }
    private Ctk.Orientation orientation = Ctk.Orientation.VERTICAL;

    private Gee.ArrayList<Clutter.Actor> children;

    private double total_child_height;

    private Gee.HashMap<Clutter.Actor, Clutter.Animation> fadeout_stack;
    private Gee.HashMap<Clutter.Actor, Clutter.Animation> fadein_stack;
    private Gee.HashMap<Clutter.Actor, Clutter.Animation> anim_stack;

    public Scroller (Ctk.Orientation orientation, int spacing) 
    {
      this.orientation = orientation;
      this.spacing = spacing;
      children = new Gee.ArrayList<Clutter.Actor> ();
      fadeout_stack = new Gee.HashMap<Clutter.Actor, Clutter.Animation> ();
      fadein_stack = new Gee.HashMap<Clutter.Actor, Clutter.Animation> ();
      anim_stack = new Gee.HashMap<Clutter.Actor, Clutter.Animation> ();

      scroll_pos = 0.0f;
    }

    construct {
      children = new Gee.ArrayList<Clutter.Actor> ();
      bgtex = new Clutter.Texture.from_file (
        Unity.PKGDATADIR + "/honeycomb.png"
        );
      assert (bgtex is Clutter.Texture);

      gradient = new Clutter.Texture.from_file (
        Unity.PKGDATADIR + "/gradient.png"
        );
      assert (gradient is Clutter.Texture);

      var mypadding = this.padding;
      
      mypadding.left = 6.0f;
      mypadding.right = 6.0f;
      mypadding.top = 6.0f;
      mypadding.bottom = 6.0f;

      this.padding = mypadding;
      
      bgtex.set_repeat (true, true);
      bgtex.set_opacity (0xA0);

      gradient.set_repeat (false, true);
      gradient.set_opacity (0x20);

      bgtex.set_parent (this);
      bgtex.queue_relayout ();
      
      gradient.set_parent (this);
      gradient.queue_relayout ();

      action_negative = new Ctk.Image.from_filename (
        24, Unity.PKGDATADIR + "/action_vert_neg.svg"
        );
      assert (action_negative is Ctk.Image);
      action_negative.set_parent (this);

      action_positive = new Ctk.Image.from_filename (
        24, Unity.PKGDATADIR + "/action_vert_pos.svg"
        );
      assert (action_positive is Ctk.Image);
      action_positive.set_parent (this);
      
      set_reactive (true);

      this.notify["scroll_pos"].connect (this.on_scrollpos_changed);
      this.on_scrollpos_changed ();
    }
    
    private void refresh_items ()
    {
      /* checks each item, if they have stepped out of the "hot" area
         their transparancy is changed and made unresponsive 
      */

      foreach (Clutter.Actor item in children)
      {
      }
    }

    private void show_hide_actions ()
    {
      refresh_items ();
      debug ("if %f > %f", total_child_height, height);
      if (total_child_height > this.height)
        {
          debug ("can scroll %f - %f", this.total_child_height, this.height);
          if (scroll_pos <= 0.0)
            {
              this.action_negative.set_opacity (0);
            } 
          else
            {
              this.action_negative.set_opacity (255);
            }
          if (scroll_pos >= 1.0)
            {
              this.action_positive.set_opacity (0);
            }
          else
            {
              this.action_positive.set_opacity (255);
            }
        }
      else 
        {
          debug ("can not scroll %f - %f", this.total_child_height, this.height);
          this.action_negative.set_opacity (0);
          this.action_positive.set_opacity (0);
        }
    }
    
    /**
     * called when the scroll_pos changes so that we always know
     * if we should have the action icon's visble
     */
    private void on_scrollpos_changed ()
    {
      debug ("scroll pos changed");
      show_hide_actions ();
    }

    private void on_fadein_completed (Clutter.Animation anim)
    {
      Clutter.Actor child = anim.get_object () as Clutter.Actor;
      this.fadein_stack.remove (child);
    }
    
    private void on_fadeout_completed (Clutter.Animation anim)
    {
      Clutter.Actor child = anim.get_object () as Clutter.Actor;
      this.fadeout_stack.remove (child);
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

        foreach (Clutter.Actor child in this.children)
        {
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
      total_child_height = 0.0f;

      if (orientation == Ctk.Orientation.VERTICAL) 
      {

        this.action_positive.get_preferred_height (for_width, 
                                                   out cmin_height, 
                                                   out cnat_height);
        
        all_child_height += cnat_height;
        
        this.action_negative.get_preferred_height (for_width, 
                                                   out cmin_height, 
                                                   out cnat_height);

        all_child_height += cnat_height;
        foreach (Clutter.Actor child in this.children)
        {
          cnat_height = 0.0f;
          cmin_height = 0.0f;
          child.get_preferred_height (for_width, 
                                      out cmin_height, 
                                      out cnat_height);
          
          all_child_height += cnat_height;
          total_child_height += cmin_height;
        }

        minimum_height = all_child_height + padding.top + padding.bottom;
        natural_height = all_child_height + padding.top + padding.bottom;
   
        //this.total_child_height = all_child_height; 
        return;
      }

      error ("Does not support Horizontal yet");

    }
                         
    public override void allocate (Clutter.ActorBox box, 
                                   Clutter.AllocationFlags flags)
    {

      base.allocate (box, flags);
      this.height = box.y2 - box.y1;

      foreach (Clutter.Actor child in this.children)
      {
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
      }

      // position the actors

      float x = padding.left;
      float y = padding.top + (-(float)this.scroll_value_as_px);

      float child_width;
      float child_height;
      Clutter.ActorBox child_box;

      //allocate the size of our action actors
      float boxwidth = 0.0f;
      action_negative.get_allocation_box (out child_box);
      boxwidth = (box.get_width () - padding.left - padding.right) 
        - child_box.get_width();
      child_box.y1 = padding.top;
      child_box.x1 = padding.left + (boxwidth / 2.0f);
      child_box.y2 = child_box.y1 + action_negative.height;
      child_box.x2 = child_box.x1 + action_negative.width;
      action_negative.allocate (child_box, flags);
      float hot_negative = child_box.y2;

      action_positive.get_allocation_box (out child_box);
      boxwidth = (box.get_width () - padding.left - padding.right) 
        - child_box.get_width();

      child_box.y2 = box.y2 - padding.bottom - 4.0f;
      child_box.y1 = child_box.y2 - action_positive.height - padding.bottom;
      child_box.x1 = padding.left + (boxwidth / 2.0f);
      child_box.x2 = child_box.x1 + action_positive.width;
      action_positive.allocate (child_box, flags);

      float hot_positive = child_box.y1;

      y = hot_negative;

      
      foreach (Clutter.Actor child in this.children)
      {
        child.get_allocation_box (out child_box);
        
        child_width = child.width;
        child_height = child.height;
        

        if (orientation == Ctk.Orientation.VERTICAL)
        {
          child_box.x1 = x;
          child_box.x2 = x + child_width + padding.right;
          child_box.y1 = y;
          child_box.y2 = y + child_height;
          
          y += child_height + spacing;
        }
        else 
        {
          error ("Does not support Horizontal yet");
        }

        // if the item is outside our "hot" area then we fade it out
        if (orientation == Ctk.Orientation.VERTICAL)
        {
          if ((child_box.y1 < hot_negative) || (child_box.y2 > hot_positive))
          {
            if (child.get_reactive ())
            {
              if (!(child in this.fadeout_stack))
              {
                var anim = child.animate (Clutter.AnimationMode.EASE_IN_QUAD, 
                                          200, "opacity", 64);
                anim.completed.connect (this.on_fadeout_completed);
                this.fadeout_stack.set (child, anim);
              }
              child.set_reactive (false);
            }
          }
          else
          {
            if (!child.get_reactive ())
            {
              if (!(child in this.fadein_stack))
              {
                var anim = child.animate (Clutter.AnimationMode.EASE_IN_QUAD,
                                          200, "opacity", 255);
                anim.completed.connect (this.on_fadein_completed);
                this.fadein_stack.set (child, anim);
              }
              child.set_reactive (true);
            }
          }
          
        }

        child.allocate (child_box, flags);

        if (y > box.y2)  
          break;
      }

      bgtex.allocate (box, flags);
      gradient.width = box.get_width();
      gradient.allocate (box, flags);
    }

    public override void pick (Clutter.Color color)
    {
      base.pick (color);

      bgtex.paint ();
      gradient.paint ();
      foreach (Clutter.Actor child in children)
      {
        child.paint ();
      }
    }

    public override void paint ()
    {
      bgtex.paint ();
      gradient.paint ();

      
      foreach (Clutter.Actor child in children) 
      {
        if ((child.flags & Clutter.ActorFlags.VISIBLE) != 0)
          child.paint ();
      }

      action_positive.paint (); 
      action_negative.paint ();
    }
    

    public void add (Clutter.Actor actor)
    {
    }

    public void add_actor (Clutter.Actor actor)
      requires (this.get_parent () != null)
    {
      this.children.add (actor);
      actor.set_parent (this);

      this.queue_relayout ();
      this.actor_added (actor);
      this.show_hide_actions ();
    }

    public void remove (Clutter.Actor actor)
    {

    }
    public void remove_actor (Clutter.Actor actor)
    {
      this.children.remove (actor);
      actor.unparent ();
      
      this.queue_relayout ();
      this.actor_removed (actor);
    }

    public void @foreach (Clutter.Callback callback, void* userdata)
    {
      foreach (Clutter.Actor child in this.children)
      {
        
        callback (child, null);
      }
    }
    
    public void foreach_with_internals (Clutter.Callback callback, 
                                        void* userdata)
    {
      callback (bgtex, null);
      callback (gradient, null);
      callback (action_positive, null);
      callback (action_negative, null);
      foreach (Clutter.Actor child in this.children)
      {
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
    public Clutter.ChildMeta get_child_meta (Clutter.Actor actor)
    {
      return null;
    }

    public void sort_depth_order ()
    {
    }
  } 
}
