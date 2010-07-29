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

  private enum ScrollerViewType
  {
    EXPANDED,
    CONTRACTED
  }

  // sucks that we have to use a class, obviously slower than we need it to be
  // but vala doesn't work with structs+gee right now.
  private class ChildTransition
  {
    public float position;
    public float rotation;
  }

  class ScrollerView : Ctk.Actor
  {
    // please don't reference this outside of this view, its only public for construct
    public ScrollerModel model {get; construct;}
    public Ctk.EffectCache cache {get; construct;}

    /* our scroller constants */
    public int spacing = 6;
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
    /*
     * state tracking variables
     */
    private bool button_down = false;
    private float total_child_height = 0.0f;
    private ScrollerPhase current_phase = ScrollerPhase.NONE;
    private uint last_motion_event_time = 0;
    private ScrollerViewType view_type = ScrollerViewType.CONTRACTED;
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

    private float contract_icon_degrees = 30.0f;
    private int focused_launcher = 0;

    /* helps out with draw order */
    private Gee.ArrayList<ScrollerChild> draw_ftb;
    private Gee.ArrayList<ScrollerChild> draw_btf;

    /*
     * Refrence holders
     */
    private Gee.ArrayList<ScrollerChild> child_refs; // we sometimes need to hold a reference to a child

    public ScrollerView (ScrollerModel _model, Ctk.EffectCache _cache)
    {
      Object (model:_model, cache:_cache);
    }

    construct
    {
      Unity.Testing.ObjectRegistry.get_default ().register ("LauncherScrollerView", this);
      var mypadding = this.padding;

      mypadding.left = 0.0f;
      mypadding.right = 0.0f;
      mypadding.top = 10.0f;
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
      enter_event.connect (on_enter_event);

      parent_set.connect (() => {
          get_stage ().motion_event.connect (on_stage_motion);
      });

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
      Idle.add (() => {
        order_children (true);
        queue_relayout ();
      });
    }

    public int get_model_index_at_y_pos_no_anim (float y, bool return_minus_if_fail=false)
    {
      SList<float?> positions = new SList<float?> ();
      foreach (ScrollerChild child in model)
        {
          positions.append (child.position);
          GLib.Value value = Value (typeof (float));
          Clutter.Animation anim = child.get_animation ();
          if (anim is Clutter.Animation)
            {
              Clutter.Interval interval = anim.get_interval ("position");
              interval.get_final_value (value);
              child.position = value.get_float ();
            }
        }

        int value = get_model_index_at_y_pos (y, return_minus_if_fail);

        unowned SList<float?> list = positions;
        foreach (ScrollerChild child in model)
          {
            child.position = (float)list.data;
            list = list.next;
          }

        return value;
    }

    public int get_model_index_at_y_pos (float y, bool return_minus_if_fail=false)
    {

      // trying out a different method
      int iy = (int)y;
      Clutter.Actor picked_actor = (get_stage () as Clutter.Stage).get_actor_at_pos (Clutter.PickMode.REACTIVE, 25, iy);
      if (picked_actor is ScrollerChild == false)
        {
          // we didn't pick a scroller child. lets pick spacing above us
          picked_actor = (get_stage () as Clutter.Stage).get_actor_at_pos (Clutter.PickMode.REACTIVE, 25, iy - spacing);

          if (picked_actor is ScrollerChild == false)
            {
              // again nothing good! lets try again but spacing below
              picked_actor = (get_stage () as Clutter.Stage).get_actor_at_pos (Clutter.PickMode.REACTIVE, 25, iy + spacing);

              if (picked_actor is ScrollerChild == false)
                {
                  if (return_minus_if_fail)
                    return -1;
                  // couldn't pick a single actor, return 0
                  return (y < padding.top + model[0].get_height () + spacing) ? 0 : model.size -1 ;
                }
            }
        }

      return model.index_of (picked_actor as ScrollerChild);

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
    }

    // will move the scroller by the given pixels
    private void move_scroll_position (float pixels)
    {
      scroll_position += pixels;
      order_children (true);
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
          Clutter.ungrab_pointer ();
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

      MenuManager manager = MenuManager.get_default ();
      manager.popdown_current_menu ();

      return true;
    }

    private bool on_enter_event (Clutter.Event event)
    {
      if (view_type == ScrollerViewType.EXPANDED) return false;
      view_type = ScrollerViewType.EXPANDED;

      // we need to set a new scroll position
      // get the index of the icon we are hovering over
      if (get_total_children_height () > get_available_height ())
        {
          int index = get_model_index_at_y_pos (event.crossing.y);

          // set our state to what we will end up being so we can find the correct
          //place to be.
          float contracted_position = model[index].position;
          var old_scroll_position = scroll_position;
          scroll_position = 0;
          order_children (true);

          float new_scroll_position = -(model[index].position - contracted_position);

          //reset our view so that we animate cleanly to the new view
          view_type = ScrollerViewType.CONTRACTED;
          scroll_position = old_scroll_position;
          order_children (true);

          // and finally animate to the new view
          view_type = ScrollerViewType.EXPANDED;

          scroll_position = new_scroll_position;
          order_children (false); // have to order twice, boo

          queue_relayout ();
        }

      return false;
    }

    private bool on_stage_motion (Clutter.Event event)
    {
      if (view_type == ScrollerViewType.CONTRACTED) return false;
      if (event.crossing.x < get_width ()) return false;
       foreach (ScrollerChild child in model)
        {
          if (child.active)
            focused_launcher = model.index_of (child);
        }

      view_type = ScrollerViewType.CONTRACTED;
      order_children (false);
      queue_relayout ();
      return false;
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

      if (button_down && is_scrolling == false && view_type != ScrollerViewType.CONTRACTED)
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
              Clutter.grab_pointer (this);
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
      Gee.ArrayList<ChildTransition> transitions;
      if (get_total_children_height () < get_available_height ())
        {
          transitions = order_children_expanded ();
        }
      else
        {
          switch (view_type)
            {
              case ScrollerViewType.CONTRACTED:
                transitions = order_children_contracted ();
                break;

              case ScrollerViewType.EXPANDED:
                transitions = order_children_expanded ();
                break;

              default:
                assert_not_reached ();
            }
        }

      for (int index = 0; index < model.size; index++)
        {
          var child = model[index];
          if (immediate)
            {
              child.position = transitions[index].position;
              child.force_rotation_jump (transitions[index].rotation);
            }
          else
            {
              bool do_new_position = true;
              if (child.get_animation () is Clutter.Animation)
                {
                  //GLib.Value value = GLib.Value (GLib.Type.from_name ("string"));
                  GLib.Value value = Value (typeof (float));
                  Clutter.Interval interval = child.get_animation ().get_interval ("position");
                  interval.get_final_value (value);
                  if (value.get_float () != transitions[index].position)
                    {
                      // disable the current animation before starting a new one
                      float current_pos = child.position;
                      child.get_animation ().completed ();
                      child.position = current_pos;
                    }
                  else
                    {
                      do_new_position = false;
                    }
                }

              child.rotation = transitions[index].rotation;

              if (do_new_position)
                child.animate (Clutter.AnimationMode.EASE_IN_OUT_QUAD,
                               300,
                               "position", transitions[index].position
                               );
            }
        }
    }

    private Gee.ArrayList<ChildTransition> order_children_expanded ()
    {
      // figures out the position of each child based on its order in the model
      Gee.ArrayList<ChildTransition> ret_transitions = new Gee.ArrayList<ChildTransition> ();
      float h = 0.0f;
      float min_height, nat_height;
      if (!(draw_ftb is Gee.ArrayList))
        draw_ftb = new Gee.ArrayList<ScrollerChild> ();
        draw_btf = new Gee.ArrayList<ScrollerChild> ();

      foreach (ScrollerChild child in model)
      {
        child.get_preferred_height (get_width (), out min_height, out nat_height);
        var transition = new ChildTransition ();
        transition.position = h + scroll_position;
        transition.rotation = 0.0f;
        ret_transitions.add (transition);
        if (!(child in draw_ftb || child in draw_ftb))
          draw_ftb.add (child);
        h += nat_height + spacing;
      }
      return ret_transitions;
    }

    private Gee.ArrayList<ChildTransition> order_children_contracted ()
    {
      Gee.ArrayList<ChildTransition> ret_transitions = new Gee.ArrayList<ChildTransition> ();
      float h = 0.0f;
      float min_height, nat_height;
      int num_launchers = 0;
      //get the total size of the children in a flat state
      float total_child_height = get_total_children_height ();

      if (total_child_height > get_available_height ())
        {
          // we need to contract some icons

          // we need to calculate how many launchers fit on our launcher
          num_launchers = (int)Math.floorf ((get_available_height () - (spacing * 2)) / (48.0f + spacing));

          for (; num_launchers >= 1; num_launchers--)
            {
              //check to see if we can fit everything in
              float flat_space = num_launchers * (48.0f + spacing);
              float contracted_space = 0.0f;
              contracted_space = ((model.size - num_launchers) * (8 + spacing));

              if (flat_space + spacing + contracted_space < (get_available_height () - (spacing * 2)))
                {
                  // everything fits in at this level, woo!
                  break;
                }
            }
          num_launchers = int.max (num_launchers, 1);
          // num_launchers now contains how many launchers should be "flat"
        }
      else
        {
          num_launchers = model.size;
        }

      int num_children_handled = 0;
      int index_start_flat, index_end_flat = 0;

      if (focused_launcher < model.size - (num_launchers -(num_launchers / 2)))
        {
          index_start_flat = int.max (0, focused_launcher - (num_launchers / 2));
          index_end_flat = index_start_flat + num_launchers;
        }
      else
        {
          index_end_flat = model.size;
          index_start_flat = index_end_flat - num_launchers;
        }
      draw_ftb = new Gee.ArrayList<ScrollerChild> ();
      draw_btf = new Gee.ArrayList<ScrollerChild> ();

      for (int index = 0; index < model.size; index++)
        {
          ScrollerChild child = model[index];
          var transition = new ChildTransition ();
          if (index >= index_start_flat && index < index_end_flat)
            {

              child.get_preferred_height (get_width (), out min_height, out nat_height);
              transition.position = h;
              h += nat_height + spacing;
              num_children_handled++;
              transition.rotation = 0.0f;

              if (index == index_start_flat)
                draw_ftb.add (child);
              else
                draw_btf.add (child);
            }
          else
            {
              // contracted launcher
              if (index == index_end_flat) h -= spacing * 2;

              transition.position = h;
              h += 8 + spacing;
              if (num_children_handled < index_start_flat)
                {
                  transition.rotation = -contract_icon_degrees;
                  draw_ftb.add (child);
                }
              else
                {
                  transition.rotation = contract_icon_degrees;
                  draw_btf.add (child);
                }
              num_children_handled++;

              if (index +1 == index_start_flat) h += 30;
            }
          ret_transitions.add (transition);
        }

      return ret_transitions;
    }


    private float get_total_children_height ()
    {
      float h = 0.0f;
      float min_height, nat_height;
      foreach (ScrollerChild child in model)
        {
          child.get_preferred_height (get_width (), out min_height, out nat_height);
          h += nat_height + spacing;
        }
      return h;
    }

    private float get_available_height ()
    {
      Clutter.ActorBox box;
      get_stored_allocation (out box);
      return box.get_height () - padding.top - padding.bottom;
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
          child_box.y1 = child.position + padding.top;
          child_box.y2 = child_box.y1 + child_height;

          child.allocate (child_box, flags);

          child.remove_clip ();
          if (child_box.y1 < 0)
            child.set_clip (0, Math.fabsf (child_box.y1),
                            child_box.get_width (), child_box.get_height () - child_box.y1);


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
    }

    public override void pick (Clutter.Color color)
    {
      base.pick (color);
      for (int index = draw_btf.size-1; index >= 0; index--)
        {
          ScrollerChild child = draw_btf[index];
          if (child is ScrollerChild && child.opacity > 0)
            {
              (child as ScrollerChild).paint ();
            }
        }

      foreach (ScrollerChild child in draw_ftb)
        {
          if (child is ScrollerChild && child.opacity > 0)
            {
              (child as ScrollerChild).paint ();
            }
        }

      foreach (ScrollerChild child in child_refs)
        {
          child.paint ();
        }
    }


    public override void paint ()
    {
      bgtex.paint ();
      for (int index = draw_btf.size-1; index >= 0; index--)
        {
          ScrollerChild child = draw_btf[index];
          if (child is ScrollerChild && child.opacity > 0)
            {
              (child as ScrollerChild).paint ();
            }
        }

      foreach (ScrollerChild child in draw_ftb)
        {
          if (child is ScrollerChild && child.opacity > 0)
            {
              (child as ScrollerChild).paint ();
            }
        }

      foreach (ScrollerChild child in child_refs)
        {
          child.paint ();
        }

      top_shadow.paint ();
    }

    public override void map ()
    {
      base.map ();
      bgtex.map ();
      top_shadow.map ();
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
      foreach (ScrollerChild child in model)
        {
          child.unmap ();
        }
    }
  }

}
