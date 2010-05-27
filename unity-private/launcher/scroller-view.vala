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
 * Authored by Gordon Allott <gord.allott@canonical.com>
 *
 */

namespace Unity.Launcher
{
  /* describes the current phase of animation for the scroller */
  private enum ScrollerPhase
  {
    PANNING,    // normal moving ar ound
    SETTLING,   // slow settling from the current position to the next one
    REORDERING, // reordering items
    FLUNG,      // flying around uncontrolled
    BOUNCE,      // bouncing back to a position
    NONE
  }

  class ScrollerView : Ctk.Actor
  {
    // please don't reference this outside of this view, its only public for construct
    public ScrollerModel model {get; construct;}

    /* our scroller constants */
    public int spacing = 2;
    public int drag_sensitivity = 7;
    public float friction = 0.9f;

    // if this is true, we don't want to be doing any gpu intensive stuff
    // until the scroller is not animating again
    public bool is_animating = false;
    /*
     * graphical images
     */
    private ThemeImage bgtex;
    private ThemeImage top_shadow;
    private ThemeImage bottom_fade;
    /*
     * state tracking variables
     */
    private bool button_down = false;
    private float total_child_height = 0.0f;
    private ScrollerPhase current_phase = ScrollerPhase.NONE;
    private uint last_motion_event_time = 0;

    /*
     * scrolling variables
     */
    private bool is_scrolling = false; //set to true when the user is phsically scrolling
    private float scroll_position = 0.0f;
    private float settle_position = 0.0f; // when we calculate the settle position for animation, we store it here

    private Clutter.Timeline fling_timeline;

    private float previous_y_position = 0.0f; // the last known y position of the pointer
    private uint previous_y_time = 0; // the time (ms) that previous_y_position was set
    private uint stored_delta = 0;
    private float scroll_speed = 0.0f; // the current speed (pixels/per second) that we are scrolling

    /*
     * Refrence holders
     */
    private Gee.ArrayList<ScrollerChild> child_refs; // we sometimes need to hold a reference to a child

    public ScrollerView (ScrollerModel _model)
    {
      Object (model:_model);
    }

    construct
    {
      Unity.Testing.ObjectRegistry.get_default ().register ("LauncherScrollerView", this);
      var mypadding = this.padding;

      mypadding.left = 0.0f;
      mypadding.right = 0.0f;
      mypadding.top = 5.0f;
      mypadding.bottom = 5.0f;

      this.padding = mypadding;

      load_textures ();
      model.child_added.connect (model_child_added);
      model.child_removed.connect (model_child_removed);
      model.order_changed.connect (model_order_changed);

      // we need to go through our model and add all the items that are currently
      // in it
      foreach (ScrollerChild child in model)
        {
          model_child_added (child);
        }

      //connect up our clutter signals
      button_press_event.connect (on_button_press_event);
      button_release_event.connect (on_button_release_event);
      motion_event.connect (on_motion_event);

      // set a timeline for our fling animation
      fling_timeline = new Clutter.Timeline (1000);
      fling_timeline.loop = true;
      fling_timeline.new_frame.connect (this.on_scroller_frame);

      //on drag start we need to disengage our own drag attempts
      var drag_controller = Drag.Controller.get_default ();
      drag_controller.drag_start.connect (() => {
        is_scrolling = false;
        button_down = false;
      });

      set_reactive (true);

      child_refs = new Gee.ArrayList <ScrollerChild> ();
      order_children (true);
      queue_relayout ();
    }

    public int get_model_index_at_y_pos (float y)
    {
      // returns the model index at the screenspace position given
      // this is slow but we can't use child.position because of animations.
      float h = 0.0f;
      float min_height, nat_height;
      int i = 0;
      foreach (ScrollerChild child in model)
        {
          float transformed_pos = h + scroll_position + padding.top;
          if (transformed_pos > y)
            return i;

          child.get_preferred_height (get_width (), out min_height, out nat_height);
          h += nat_height + spacing;
          i++;
        }

      return int.max (i, 0);
    }

    /*
     * private methods
     */
    private void load_textures ()
    {
      bgtex = new ThemeImage ("launcher_background_middle");
      bgtex.set_repeat (true, true);
      bgtex.set_parent (this);

      top_shadow = new ThemeImage ("overflow_top");
      top_shadow.set_repeat (true, false);
      top_shadow.set_parent (this);

      bottom_fade = new ThemeImage ("overflow_bottom");
      bottom_fade.set_repeat (true, false);
      bottom_fade.set_parent (this);
    }

    // will move the scroller by the given pixels
    private void move_scroll_position (float pixels)
    {
      scroll_position += pixels;
      queue_relayout ();
    }

    /* disables animations and events on children so that they don't
     * get in the way of our scroller interactions
     */
    private void disable_animations_on_children (Clutter.Event event)
    {
      Clutter.Event e = { 0 };
      e.type = Clutter.EventType.LEAVE;
      e.crossing.time = event.motion.time;
      e.crossing.flags = event.motion.flags;
      e.crossing.stage = event.motion.stage;
      e.crossing.x = event.motion.x;
      e.crossing.y = event.motion.y;

      foreach (ScrollerChild child in model)
        {
          if (child is Clutter.Actor)
            {
              e.crossing.source = child;
              child.do_event (e, false);
            }
        }
    }

    /*
     * model signal connections
     */
    private void model_child_added (ScrollerChild child)
    {
      child.unparent ();
      child.set_parent (this);

      // we only animate if the added child is not at the end
      if (model.index_of (child) == model.size -1)
        order_children (true);
      else
        order_children (false);
      queue_relayout ();
      child.notify["position"].connect (() => {
        queue_relayout ();
      });
    }

    private void model_child_removed (ScrollerChild child)
    {
      child_refs.add (child); // we need to keep a reference on it for now
      var anim = child.animate (Clutter.AnimationMode.EASE_OUT_QUAD,
                                SHORT_DELAY,
                                "opacity", 0);
      anim.completed.connect (() => {
        child.unparent ();
        child_refs.remove (child);
      });

      order_children (false);
      queue_relayout ();
    }

    private void model_order_changed ()
    {
      order_children (false);
      queue_relayout ();
    }

    /*
     * Clutter signal connections
     */
    private bool on_button_press_event (Clutter.Event event)
    {
      if (event.button.button != 1)
        {
          // not a left click
          return false;
        }

      //Clutter.grab_pointer (this);
      button_down = true;
      previous_y_position = event.button.y;
      previous_y_time = event.button.time;

      this.get_stage ().button_release_event.connect (this.on_button_release_event);

      return true;
    }

    private bool on_button_release_event (Clutter.Event event)
    {

      if (event.button.button != 1)
        {
          // not a left click
          return false;
        }

      //Clutter.ungrab_pointer ();
      button_down = false;
      this.get_stage ().button_release_event.disconnect (this.on_button_release_event);
      Unity.global_shell.remove_fullscreen_request (this);

      if (is_scrolling)
        {
          is_scrolling = false;
          get_stage ().motion_event.disconnect (on_motion_event);
          if ((event.button.time - last_motion_event_time) > 120)
            {
              current_phase = ScrollerPhase.SETTLING;
              settle_position = get_aligned_settle_position ();
            }
          else
            {
              current_phase = ScrollerPhase.FLUNG;
            }

          foreach (ScrollerChild child in model)
            {
              child.set_reactive (false);
            }
          fling_timeline.start ();
        }
      return true;
    }

    private bool on_motion_event (Clutter.Event event)
    {
      var drag_controller = Drag.Controller.get_default ();
      if (drag_controller.is_dragging)
      {
        // we are dragging from somewhere else, ignore motion events
        return false;
      }

      last_motion_event_time = event.motion.time;

      if (button_down && is_scrolling == false)
        {
          /* we have a left button down, but we aren't dragging yet, we need to
           * monitor how far away we have dragged from the original click, once
           * we get far enough away we can start scrolling.
           */
          var diff = event.motion.y - previous_y_position;
          if (Math.fabsf (diff) > drag_sensitivity)
            {
              is_scrolling = true;
              Unity.global_shell.add_fullscreen_request (this);
              get_stage ().motion_event.connect (on_motion_event);
            }
        }

      if (is_scrolling)
        {
          /* Disable any animations on the children */
          disable_animations_on_children (event);

          /* we need to compare the event y position from this event to the
           * previous event. once we have that we can compute a velocity based
           * on how long it was since the previous event
           */

          float pixel_diff = event.motion.y - previous_y_position;
          uint time_diff = event.motion.time - previous_y_time;
          scroll_speed = pixel_diff / (time_diff / 1000.0f);

          previous_y_position = event.motion.y;
          previous_y_time = event.motion.time;

          // move the scroller by how far we dragged
          move_scroll_position (pixel_diff);

          return true;
        }

      return false;
    }

    /*
     * Methods to handle our scroller animation, generally todo with the scrolling
     */
    private void on_scroller_frame (Clutter.Timeline timeline, int msecs)
    {
      /* one of the things about our scroller is that we really need to step the
       * dynamically simulated parts of the animation at a fixed framerate. but
       * clutter does not allow that. so we fake it :) this code aims for 60/fps
       */
      // animate the scroller depeding on its phase
      uint delta = timeline.get_delta ();
      delta += stored_delta;
      if (delta <= 16)
        {
          stored_delta = delta;
          return;
        }

      while (delta > 16)
        {
          is_animating = true;
          delta -= 16;
          switch (current_phase) {
            case (ScrollerPhase.SETTLING):
              do_anim_settle (timeline, msecs);
              break;
            case (ScrollerPhase.FLUNG):
              do_anim_fling (timeline, msecs);
              break;
            case (ScrollerPhase.BOUNCE):
              do_anim_bounce (timeline, msecs);
              break;
            case (ScrollerPhase.NONE):
              timeline.stop ();
              scroll_speed = 0.0f;
              is_animating = false;
              foreach (ScrollerChild child in model)
                {
                  child.set_reactive (true);
                }
              break;
            default:
              assert_not_reached ();
          }
        }

      stored_delta = delta;
    }

    private void do_anim_settle (Clutter.Timeline timeline, int msecs)
    {
      var distance = settle_position - scroll_position;
      move_scroll_position (distance * 0.2f);
      if (Math.fabs (distance) < 1 )
        {
          current_phase = ScrollerPhase.NONE;
        }

    }

    private void do_anim_fling (Clutter.Timeline timeline, int msecs)
    {
      scroll_speed *= friction; // slow our speed

      // we devide by 60 because get 60 ticks a second
      float scroll_move_amount = scroll_speed / 60.0f;
      move_scroll_position (scroll_move_amount);

      //after a fling, we have to figure out if we want to change our
      // scroller phase or not

      if(scroll_move_amount <= -1.0 && -scroll_position > total_child_height - height ||
         scroll_move_amount >=  1.0 && scroll_position > 0)
        {
          current_phase = ScrollerPhase.BOUNCE;
        }

      if (Math.fabsf (scroll_move_amount) < 1.0 &&
          (scroll_position > 0 || -scroll_position > total_child_height - height))
        {
          settle_position = get_aligned_settle_position ();
          current_phase = ScrollerPhase.SETTLING;
        }
      else if (Math.fabsf (scroll_move_amount) < 1.0)
        {
          current_phase = ScrollerPhase.NONE;
        }
    }

    private void do_anim_bounce (Clutter.Timeline timeline, int msecs)
    {
      scroll_speed *= 0.5f;
      move_scroll_position (scroll_speed / 60.0f);
          settle_position = -get_aligned_settle_position ();
      current_phase = ScrollerPhase.SETTLING;
    }

    private float get_aligned_settle_position ()
    {
      /* attempts to integligently find the correct settle position */
      float final_position = scroll_position;
      if (total_child_height < height - padding.top - padding.bottom)
        {
          // just move up to the top because we don't have enough items
          final_position = 0;
        }
      else if (scroll_position > 0)
        {
          // we always position on the first child
          final_position = 0;
        }
      else if (-scroll_position > total_child_height - height - padding.top - padding.bottom)
        {
          // position on the final child
          final_position = total_child_height - height + padding.bottom;
        }

      return final_position;
    }


    /*
     * Clutter overrides
     */
    public override void get_preferred_width (float for_height,
                                              out float minimum_width,
                                              out float natural_width)
    {
      minimum_width = 0;
      natural_width = 0;

      float pmin_width = 0.0f;
      float pnat_width = 0.0f;

      foreach (ScrollerChild child in model)
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

    }

    public override void get_preferred_height (float for_width,
                                      out float minimum_height,
                                      out float natural_height)
    {
      minimum_height = 0.0f;
      natural_height = 0.0f;

      float cnat_height = 0.0f;
      float cmin_height = 0.0f;
      total_child_height = 0.0f;

      foreach (ScrollerChild child in model)
      {
        cnat_height = 0.0f;
        cmin_height = 0.0f;
        child.get_preferred_height (for_width,
                                    out cmin_height,
                                    out cnat_height);
        total_child_height += cnat_height + spacing;
      }

      minimum_height = total_child_height + padding.top + padding.bottom;
      natural_height = total_child_height + padding.top + padding.bottom;
      return;
    }

    private void order_children (bool immediate)
    {
      // figures out the position of each child based on its order in the model
      float h = 0.0f;
      float min_height, nat_height;
      foreach (ScrollerChild child in model)
      {
        child.get_preferred_height (get_width (), out min_height, out nat_height);
        if (h != child.position)
        {
          if (!immediate)
            {
              if (child.get_animation () is Clutter.Animation)
                {
                  float current_pos = child.position;
                  child.get_animation ().completed ();
                  child.position = current_pos;
                }

              child.animate (Clutter.AnimationMode.EASE_IN_OUT_QUAD,
                             170,
                             "position", h);
            }
          else
            {
              child.position = h;
            }
        }

        h += nat_height + spacing;
      }
    }

    public override void allocate (Clutter.ActorBox box,
                                   Clutter.AllocationFlags flags)
    {
      base.allocate (box, flags);
      Clutter.ActorBox child_box = Clutter.ActorBox ();
      float current_width = padding.left;
      float available_height = box.get_height () - padding.bottom;
      float available_width = box.get_width () - padding.right;

      total_child_height = 0.0f;

      foreach (ScrollerChild child in model)
        {
          float child_height, child_width, natural, min;

          child.get_preferred_width (available_height, out min, out natural);
          child_width = Math.fmaxf (min, Math.fminf (natural, available_width));

          child.get_preferred_height (child_width, out min, out natural);
          child_height = Math.fmaxf (min, Math.fminf (natural, available_height));

          child_box.x1 = current_width;
          child_box.x2 = box.get_width () - padding.right;
          child_box.y1 = child.position + padding.top + scroll_position;
          child_box.y2 = child_box.y1 + child_height;

          child.allocate (child_box, flags);

          total_child_height += child_height + spacing;
        }

      child_box.x1 = 0;
      child_box.x2 = box.get_width ();

      /* allocate the background graphic */
      int bg_height, bg_width;
      bgtex.get_base_size (out bg_width, out bg_height);
      float bg_offset = Math.fmodf (scroll_position + 1000000, bg_height);

      child_box.y1 = bg_offset - (bg_height - 1);
      child_box.y2 = bg_offset + (bg_height - 1) + box.get_height ();

      bgtex.allocate (child_box, flags);

      /* allocate the extra graphics */
      top_shadow.get_base_size (out bg_width, out bg_height);
      child_box.y1 = -1;
      child_box.y2 = bg_height -1;
      top_shadow.allocate (child_box, flags);

      bottom_fade.get_base_size (out bg_width, out bg_height);
      child_box.y1 = box.get_height() - bg_height;
      child_box.y2 = box.get_height();
      bottom_fade.allocate (child_box, flags);
    }

    public override void pick (Clutter.Color color)
    {
      base.pick (color);
      foreach (ScrollerChild child in model)
        {
          child.paint ();
        }
    }


    public override void paint ()
    {
      bgtex.paint ();
      foreach (ScrollerChild child in model)
        {
          if (child is LauncherChild && child.opacity > 0)
            {
              (child as LauncherChild).paint ();
            }
        }

      foreach (ScrollerChild child in child_refs)
        {
          child.paint ();
        }

      top_shadow.paint ();
      bottom_fade.paint ();
    }

    public override void map ()
    {
      base.map ();
      bgtex.map ();
      top_shadow.map ();
      bottom_fade.map ();
      foreach (ScrollerChild child in model)
        {
          child.map ();
        }
    }

    public override void unmap ()
    {
      base.unmap ();
      bgtex.map ();
      top_shadow.map ();
      bottom_fade.map ();
      foreach (ScrollerChild child in model)
        {
          child.unmap ();
        }
    }
  }

}
