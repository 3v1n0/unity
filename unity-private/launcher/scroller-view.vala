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
    private bool disable_child_events = false;
    // please don't reference this outside of this view, its only public for construct
    public ScrollerModel model {get; construct;}
    public Ctk.EffectCache cache {get; construct;}

    public bool drag_indicator_active {get; set;}
    public bool drag_indicator_space {get; set;}
    public int drag_indicator_index {get; set;}
    public float drag_indicator_opacity {get; set;}
    private float drag_indicator_position = 0.0f;


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
    private ScrollerPhase current_phase = ScrollerPhase.SETTLING;
    private uint last_motion_event_time = 0;
    private ScrollerViewType view_type = ScrollerViewType.CONTRACTED;
    private bool do_logic_pick = true;
    private float last_known_pointer_x = 0.0f;
    private bool can_scroll = false;

    private float last_known_x = 0;
    private float last_known_y = 0;

    /*
     * scrolling variables
     */
    private bool is_scrolling; //set to true when the user is physically scrolling
    private float scroll_position = 0.0f;
    private float settle_position = 0.0f; // when we calculate the settle position for animation, we store it here

    public bool is_autoscrolling {get; set;} //set to true when the mouse is at the top/bottom
    private bool autoscroll_anim_active = false;
    private int autoscroll_direction = 0;

    public Clutter.Timeline fling_timeline;

    public Clutter.Timeline autoscroll_timeline;

    private float previous_y_position = 0.0f; // the last known y position of the pointer
    private uint previous_y_time = 0; // the time (ms) that previous_y_position was set
    private uint stored_delta = 0;
    private float scroll_speed = 0.0f; // the current speed (pixels/per second) that we are scrolling

    private float contract_icon_degrees = 70.0f;
    private float contract_icon_partial_degrees = 30.0f;
    private int focused_launcher = 0;

    /* helps out with draw order */
    private Gee.ArrayList<ScrollerChild> draw_ftb;
    private Gee.ArrayList<ScrollerChild> draw_btf;

    /* Key binding indicators */
    private Gee.ArrayList<Clutter.CairoTexture> keyboard_indicators;

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
      motion_event.connect (passthrough_motion_event);
      button_press_event.connect (passthrough_button_press_event);
      button_release_event.connect (passthrough_button_release_event);

      Unity.Testing.ObjectRegistry.get_default ().register ("LauncherScrollerView", this);
      var mypadding = this.padding;

      mypadding.left = 0.0f;
      mypadding.right = 0.0f;
      mypadding.top = 10.0f;
      mypadding.bottom = 5.0f;

      this.padding = mypadding;

      keyboard_indicators = new Gee.ArrayList <Clutter.CairoTexture> ();
      load_textures ();
      Clutter.Color color = Clutter.Color () {
        red = 0xff,
        green = 0xff,
        blue = 0xff,
        alpha = 0xff
      };

      notify["drag-indicator-active"].connect (on_drag_indicator_active_change);
      notify["drag-indicator-index"].connect (on_drag_indicator_index_change);
      notify["drag-indicator-space"].connect (on_drag_indicator_space_change);

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
      scroll_event.connect (on_scroll_event);

      leave_event.connect (on_leave_event);
      notify["is-autoscrolling"].connect (on_auto_scrolling_state_change);
      Unity.Drag.Controller.get_default ().drag_motion.connect (on_drag_motion_event);

      // set a timeline for our fling animation
      fling_timeline = new Clutter.Timeline (1000);
      fling_timeline.loop = true;
      fling_timeline.new_frame.connect (this.on_scroller_frame);
      fling_timeline.started.connect (() => {
        cache.invalidate_texture_cache ();
      });
      fling_timeline.completed.connect (() => {
        Timeout.add (0, () => {
        cache.update_texture_cache ();
        return false;
        });
      });

      // set a timeline for our auto-scroll animation
      autoscroll_timeline = new Clutter.Timeline (1000);
      autoscroll_timeline.loop = true;
      autoscroll_timeline.new_frame.connect (this.on_autoscroll_frame);
      autoscroll_timeline.started.connect (() => {
        cache.invalidate_texture_cache ();
      });

      autoscroll_timeline.completed.connect (() => {
        Timeout.add (0, () => {
          cache.update_texture_cache ();
          return false;
        });
      });

      //on drag start we need to disengage our own drag attempts
      var drag_controller = Drag.Controller.get_default ();
      drag_controller.drag_start.connect (() => {
        is_scrolling = false;
        button_down = false;
        Unity.Launcher.disable_quicklists = true;
        Clutter.ungrab_pointer ();
        get_stage ().motion_event.disconnect (on_motion_event);
        current_phase = ScrollerPhase.FLUNG;
        fling_timeline.start ();

        animate (Clutter.AnimationMode.EASE_OUT_SINE, 150,
                 "drag-indicator-opacity", 1.0f);
      });

      drag_controller.drag_drop.connect ((drag_model, x, y) => {
        Unity.Launcher.disable_quicklists = false;
        foreach (Clutter.Actor child in model)
          {
            child.set_reactive (false);
          }
          if (x > get_width ()) contract_launcher ();
          else
            {
              Idle.add (() => {
                order_children (false);
              });
            }
          animate (Clutter.AnimationMode.EASE_OUT_SINE, 150,
                   "drag-indicator-opacity", 0.0f);
      });

      drag_controller.drag_motion.connect ((model, x, y) => {
        last_known_pointer_x = x;
        if (x > 200 + get_width ()) contract_launcher ();
        else expand_launcher (y);
      });

      set_reactive (true);

      child_refs = new Gee.ArrayList <ScrollerChild> ();
      order_children (true);
      queue_relayout ();
      Idle.add (() => {
        order_children (true);
        queue_relayout ();
      });

      drag_indicator_space = false;

    }

    /* hoo-boy this sucks. because of mutter and clutter issues, i have to set
     * all my children to be non reactive. then i need to go through and
     * send events to the correct children myself, by-passing clutter
     * all this so i can accurately know if a mouse-leave event on the launcher
     * is real... sheesh
     */
    private Clutter.Actor? last_picked_actor = null;
    private Clutter.Actor? handle_event (Clutter.Event event, bool assume_on_launcher=false)
    {
      if (disable_child_events)
        return null;

      if ((last_picked_actor is Clutter.Actor) == false)
        last_picked_actor = null;


      foreach (Clutter.Actor actor in model)
        {
          actor.set_reactive (true);
        }


      float x, y;
      event.get_coords (out x, out y);
      last_known_x = x;
      last_known_y = y;
      if (assume_on_launcher)
        x = 25;

      Clutter.Actor picked_actor = (get_stage () as Clutter.Stage).get_actor_at_pos (Clutter.PickMode.REACTIVE, (int)x, (int)y);


      foreach (Clutter.Actor actor in model)
        {
          actor.set_reactive (false);
        }


      if (picked_actor is Clutter.Actor)
        {
          // we now have a picked actor, figure out what event to send it
          if (last_picked_actor != picked_actor)
            {
              Clutter.Event crossing_event =  { 0 };
              crossing_event.type = Clutter.EventType.LEAVE;
              crossing_event.crossing.x = x;
              crossing_event.crossing.y = y;
              crossing_event.crossing.stage = get_stage () as Clutter.Stage;
              crossing_event.crossing.flags = Clutter.EventFlags.FLAG_SYNTHETIC;

              if (last_picked_actor is Clutter.Actor)
                last_picked_actor.do_event (crossing_event, false);

              crossing_event.type = Clutter.EventType.ENTER;
              picked_actor.do_event (crossing_event, false);
            }
        }
      else if (last_picked_actor is Clutter.Actor)
        {
          // if picked_actor is null, then we want to send a leave event on the
          // previous actor

          Clutter.Event crossing_event =  { 0 };
          crossing_event.type = Clutter.EventType.LEAVE;
          crossing_event.crossing.x = x;
          crossing_event.crossing.y = y;
          crossing_event.crossing.stage = get_stage () as Clutter.Stage;
          crossing_event.crossing.flags = Clutter.EventFlags.FLAG_SYNTHETIC;

          last_picked_actor.do_event (crossing_event, false);
        }

      last_picked_actor = picked_actor;
      return picked_actor;
    }

    private bool passthrough_motion_event (Clutter.Event event)
    {
      var drag_controller = Drag.Controller.get_default ();
      if (drag_controller.is_dragging) return false;
      enter_event.disconnect (on_enter_event);
      leave_event.disconnect (on_leave_event);
      motion_event.disconnect (on_motion_event);
      motion_event.disconnect (passthrough_motion_event);
      if (is_scrolling)
        {
          get_stage ().motion_event.disconnect (on_motion_event);
        }

      Clutter.Actor picked_actor = handle_event (event, is_scrolling);

      if (picked_actor is Clutter.Actor)
          picked_actor.do_event (event, false);

      enter_event.connect (on_enter_event);
      leave_event.connect (on_leave_event);
      motion_event.connect (on_motion_event);
      motion_event.connect (passthrough_motion_event);
      if (is_scrolling)
        {
          get_stage ().motion_event.connect (on_motion_event);
        }
      return false;
    }

    private bool passthrough_button_press_event (Clutter.Event event)
    {
      var drag_controller = Drag.Controller.get_default ();
      if (drag_controller.is_dragging) return false;

      enter_event.disconnect (on_enter_event);
      leave_event.disconnect (on_leave_event);
      button_press_event.disconnect (passthrough_button_press_event);
      Clutter.Actor picked_actor = handle_event (event, is_scrolling);
      if (picked_actor is Clutter.Actor)
        picked_actor.do_event (event, false);

      enter_event.connect (on_enter_event);
      leave_event.connect (on_leave_event);
      button_press_event.connect (passthrough_button_press_event);
      return false;
    }

    private bool passthrough_button_release_event (Clutter.Event event)
    {
      var drag_controller = Drag.Controller.get_default ();
      if (drag_controller.is_dragging) return false;

      enter_event.disconnect (on_enter_event);
      leave_event.disconnect (on_leave_event);
      button_release_event.disconnect (passthrough_button_release_event);
      Clutter.Actor picked_actor = handle_event (event, is_scrolling);
      if (picked_actor is Clutter.Actor)
        picked_actor.do_event (event, false);

      enter_event.connect (on_enter_event);
      leave_event.connect (on_leave_event);
      button_release_event.connect (passthrough_button_release_event);
      return false;
    }

    private void on_drag_indicator_space_change ()
    {
      if (drag_indicator_active)
        {
          if (drag_indicator_space)
            {
              animate (Clutter.AnimationMode.EASE_OUT_SINE, 150,
                       "drag-indicator-opacity", 0.0f);
              order_children (false);
              return;
            }
          else
            {
              animate (Clutter.AnimationMode.EASE_OUT_SINE, 150,
                       "drag-indicator-opacity", 1.0f);
              order_children (false);
            }
        }
      else
        {
          animate (Clutter.AnimationMode.EASE_OUT_SINE, 150,
                   "drag-indicator-opacity", 1.0f);
          order_children (false);
        }
    }

    private void on_drag_indicator_active_change ()
    {
      on_drag_indicator_space_change ();
    }

    private void on_drag_indicator_index_change ()
    {
      order_children (false);
      queue_relayout ();
    }

    private float last_scroll_position = 0.0f;
    public void enable_keyboard_selection_mode (bool choice)
    {
      if (choice)
        last_scroll_position = scroll_position;

      uint8 new_opacity = (choice) ? 0xff : 0x00;

      int index = 1;
      foreach (Clutter.CairoTexture kb_ind in keyboard_indicators)
        {
          kb_ind.animate (Clutter.AnimationMode.EASE_OUT_SINE, 150,
                          "opacity", new_opacity);
          if (model.size <= index) new_opacity = 0x00;
          index++;
        }


      if (!choice)
        {
          if (view_type != ScrollerViewType.CONTRACTED &&
              last_known_pointer_x >= get_width ())
            {
              foreach (ScrollerChild child in model)
              {
                if (child.active)
                  {
                    focused_launcher = model.index_of (child);
                    break;
                  }
              }
              contract_launcher ();
            }
          else if (last_known_pointer_x < get_width ())
            {
              move_scroll_position (last_scroll_position - scroll_position);
            }
        }
      else
        {
          expand_launcher (0);
        }


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
              if (interval is Clutter.Interval)
                {
                  interval.get_final_value (value);
                  child.position = value.get_float ();
                }
            }
        }

        int value = get_model_index_at_y_pos (y, return_minus_if_fail);

        unowned SList<float?> list = positions;
        foreach (ScrollerChild child in model)
          {
            child.position = (float)list.data;
            list = list.next;
          }

        ScrollerChild child = (Drag.Controller.get_default ().get_drag_model () as ScrollerChildController).child;

        //value = model.clamp (child, value);

        return value;
    }

    public int get_model_index_at_y_pos (float y, bool return_minus_if_fail=false)
    {
      if (!do_logic_pick)
        return get_model_index_at_y_pos_pick (y, return_minus_if_fail);
      else
        return get_model_index_at_y_pos_logic (y, return_minus_if_fail);
    }

    private int get_model_index_at_y_pos_logic (float y, bool return_minus_if_fail=false)
    {
      foreach (ScrollerChild child in model)
        {
          if (child.position + padding.top + child.get_height () > y)
            return model.index_of (child as ScrollerChild);
        }

      if (return_minus_if_fail)
        return -1;

      return (y < padding.top + model[0].get_height () + spacing) ? 0 : model.size -1;

    }

    private int get_model_index_at_y_pos_pick(float y, bool return_minus_if_fail=false)
    {
      // trying out a different method
      int iy = (int)y;
      foreach (ScrollerChild actor in model)
        {
          if (!actor.do_not_render)
            actor.set_reactive (true);
        }

      Clutter.Actor picked_actor = (get_stage () as Clutter.Stage).get_actor_at_pos (Clutter.PickMode.REACTIVE, 25, iy);
      int ret_val = -200;

      if (picked_actor is ScrollerChild == false)
        {
          // we didn't pick a scroller child. lets pick below us
          picked_actor = (get_stage () as Clutter.Stage).get_actor_at_pos (Clutter.PickMode.REACTIVE, 25, iy - 24);

          if (picked_actor is ScrollerChild == false)
            {
              // again nothing good! lets try again above us
              picked_actor = (get_stage () as Clutter.Stage).get_actor_at_pos (Clutter.PickMode.REACTIVE, 25, iy + 24);
              if (picked_actor is ScrollerChild == false)
                {
                  if (return_minus_if_fail)
                    ret_val = -1;
                  // couldn't pick a single actor, return 0
                  ret_val =  (y < padding.top + model[0].get_height () + spacing) ? 0 : model.size -1 ;
                }
            }
        }

      if (ret_val < -1)
        ret_val = model.index_of (picked_actor as ScrollerChild);

      foreach (Clutter.Actor actor in model)
        {
          actor.set_reactive (false);
        }

      return ret_val;
    }

    private void draw_keyboard_indicator_cairo (Cairo.Context cr, string text)
    {
      double x = 0;
      double y = 0;
      double w = 10;
      double h = 10;
      double r = Ctk.em_to_pixel (1);

      Gtk.Settings settings = Gtk.Settings.get_default ();
      Pango.FontDescription desc = Pango.FontDescription.from_string (settings.gtk_font_name);

      cr.select_font_face (desc.get_family (),
                           Cairo.FontSlant.NORMAL,
                           Cairo.FontWeight.NORMAL);
      double size;
      size = Ctk.em_to_pixel (1) * 0.9;
      cr.set_font_size (size);


      Cairo.TextExtents extents = Cairo.TextExtents ();
      cr.text_extents ("2", out extents);

      Cairo.TextExtents real_extents = Cairo.TextExtents ();
      cr.text_extents (text, out extents);

      w += extents.width;
      h += extents.height;
      cr.set_source_rgba (0.07, 0.07, 0.07, 0.8);

      cr.move_to(x+r,y);                      // Move to A
      cr.line_to(x+w-r,y);                    // Straight line to B
      cr.curve_to(x+w,y,x+w,y,x+w,y+r);       // Curve to C, Control points are both at Q
      cr.line_to(x+w,y+h-r);                  // Move to D
      cr.curve_to(x+w,y+h,x+w,y+h,x+w-r,y+h); // Curve to E
      cr.line_to(x+r,y+h);                    // Line to F
      cr.curve_to(x,y+h,x,y+h,x,y+h-r);       // Curve to G
      cr.line_to(x,y+r);                      // Line to H
      cr.curve_to(x,y,x,y,x+r,y);             // Curve to

      cr.fill ();

      //x = (extents.width - real_extents.width) / 2.0;
      y = 0;//(extents.height - real_extents.height) / 2.0;
      cr.set_source_rgba (1, 1, 1, 1);
      cr.move_to (x + 5 - (real_extents.width * 0.5), y+5+extents.height);
      cr.show_text (text);
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

      var color = Clutter.Color () {
        red = 0xff,
        green = 0xff,
        blue = 0xff,
        alpha = 0xff
      };

      int index = 1;
      // indicator size find out activate!
      int key_indicator_w, key_indicator_h;
      Gtk.Settings settings = Gtk.Settings.get_default ();

      Unity.QuicklistRendering.get_text_extents (settings.gtk_font_name, "2",
                                                 out key_indicator_w, out key_indicator_h);

      key_indicator_w += 10;
      key_indicator_h += 10;

      for (; index <= 10; index++)
        {
          var keyboard_indicator = new Clutter.CairoTexture (key_indicator_w, key_indicator_h);
          keyboard_indicator.set_parent (this);
          keyboard_indicator.opacity = 0x00;

          keyboard_indicator.set_surface_size (key_indicator_w, key_indicator_h);
          keyboard_indicator.set_size (key_indicator_w, key_indicator_h);
          keyboard_indicator.clear ();
            {
              Cairo.Context cr = keyboard_indicator.create ();

              string ind_str = index.to_string ();
              if (index == 10)
                ind_str = "0";

              draw_keyboard_indicator_cairo (cr, ind_str);
            }
          keyboard_indicators.add (keyboard_indicator);
        }
    }

    // will move the scroller by the given pixels
    private float calculate_scroll_position (bool check_bounds=false, float limit = 160.0f)
    {
      float new_scroll_position = scroll_position;
      if (check_bounds)
        {
          new_scroll_position = Math.fminf (new_scroll_position, 0);
          new_scroll_position = Math.fmaxf (new_scroll_position, - (get_total_children_height () - get_available_height ()));
        }
      else if (new_scroll_position > 0)
        {
          new_scroll_position = limit * ( 1 - Math.powf ((limit - 1) / limit, new_scroll_position));
        }
      else if (get_total_children_height () < get_available_height () &&
               new_scroll_position < 0)
        {
          new_scroll_position = -new_scroll_position;
          new_scroll_position = limit * ( 1 - Math.powf ((limit - 1) / limit, new_scroll_position));
          new_scroll_position = -new_scroll_position;
        }
      else if (get_total_children_height () >= get_available_height () &&
               new_scroll_position < -(get_total_children_height () - get_available_height ()))
        {
          float diff = new_scroll_position + (get_total_children_height () - get_available_height ());
          new_scroll_position = limit * ( 1 - Math.powf ((limit - 1) / limit, Math.fabsf (diff)));
          new_scroll_position = -(get_total_children_height () - get_available_height ()) - new_scroll_position;
        }

      return new_scroll_position;
    }

    private void move_scroll_position (float pixels, bool check_bounds=false, float limit = 160.0f)
    {
      scroll_position += pixels;
      float old_scroll_position = scroll_position;

      scroll_position = calculate_scroll_position (check_bounds, limit);

      order_children (true);
      queue_relayout ();

      scroll_position = old_scroll_position;
    }

    /* disables animations and events on children so that they don't
     * get in the way of our scroller interactions
     */
    private void disable_animations_on_children (Clutter.Event event)
    {
/*
      disable_child_events = true;

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
*/

    }

    private void expand_launcher (float absolute_y)
    {
      if (view_type == ScrollerViewType.EXPANDED) return;
      view_type = ScrollerViewType.EXPANDED;

      // we need to set a new scroll position
      // get the index of the icon we are hovering over
      if (get_total_children_height () > get_available_height ())
        {
          do_logic_pick = false;
          int index = get_model_index_at_y_pos (absolute_y);

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
          do_logic_pick = true;
        }
    }


    private void contract_launcher ()
    {
      if (view_type == ScrollerViewType.CONTRACTED) return;

      foreach (ScrollerChild child in model)
        {
          if (child.active)
            focused_launcher = model.index_of (child);
        }

      view_type = ScrollerViewType.CONTRACTED;
      order_children (false);
      queue_relayout ();
      is_autoscrolling = false;
    }

    /*
     * model signal connections
     */
    private void model_child_added (ScrollerChild child)
    {
      child.unparent ();
      child.set_parent (this);

      // we only animate if the added child is not at the end
      float[] prev_positions = {};//new float [model.size];
      float[] prev_rotations = {};//new float [model.size];

      foreach (ScrollerChild modelchild in model)
        {
          prev_positions += modelchild.position;
          prev_rotations += modelchild.rotation;
        }
      order_children (true);

      int index = 0;
      foreach (ScrollerChild modelchild in model)
        {
          if (child != modelchild)
            {
              change_child_position_rotation (modelchild,
                                              prev_positions[index],
                                              prev_rotations[index],
                                              true);
            }
          index++;
        }

      order_children (false);
      queue_relayout ();
      child.notify["position"].connect (() => {
        queue_relayout ();
      });

      child.set_reactive (false);
    }

    private void model_child_removed (ScrollerChild child)
    {
      if (child in draw_btf) draw_btf.remove (child);
      if (child in draw_ftb) draw_ftb.remove (child);

      var drag_controller = Drag.Controller.get_default ();
      if (drag_controller.is_dragging)
        {
          order_children (false);
          queue_relayout ();
        }
      else
        {
          child_refs.add (child); // we need to keep a reference on it for now
          var anim = child.animate (Clutter.AnimationMode.EASE_OUT_QUAD,
                                    SHORT_DELAY,
                                    "opacity", 0);

          var icon_scale_anim = child.processed_icon.animate (Clutter.AnimationMode.EASE_OUT_QUAD,
                                                              SHORT_DELAY,
                                                              "scale-x", 0.0,
                                                              "scale-y", 0.0);
          anim.completed.connect (() => {
            child.unparent ();
            child_refs.remove (child);
            order_children (false);
            queue_relayout ();
          });
        }
    }

    private void model_order_changed ()
    {
      order_children (false);
      queue_relayout ();
    }

    private uint autoscroll_stored_delta = 0;
    private void on_autoscroll_frame (Clutter.Timeline timeline, int msecs)
    {
      /* using a clutter animation to handle auto-scroll now
       * doesn't fit the formula from design properly but thats just a fact
       * of the animation systems being different
       */
      uint delta = timeline.get_delta ();
      delta += autoscroll_stored_delta;
      if (delta <= 16)
        {
          autoscroll_stored_delta = delta;
          return;
        }

      while (delta > 33)
        {
          delta -= 33;
          float speed = 0.0f;
          if (autoscroll_mouse_pos_cache < 0)
            speed = Math.fabsf (-12 - autoscroll_mouse_pos_cache);
          else
            speed = 12.0f - Math.fabsf (autoscroll_mouse_pos_cache);

          speed /= 12.0f;
          speed *= 30;
          speed *= autoscroll_direction;
          move_scroll_position (speed, true);
        }

        Clutter.Event motion_event =  { 0 };
        motion_event.type = Clutter.EventType.MOTION;
        motion_event.motion.x = last_known_x;
        motion_event.motion.y = last_known_y;
        motion_event.motion.stage = get_stage () as Clutter.Stage;
        motion_event.motion.flags = Clutter.EventFlags.FLAG_SYNTHETIC;
        passthrough_motion_event (motion_event);

        autoscroll_stored_delta = delta;
    }

    private void on_auto_scrolling_state_change ()
    {
      if (autoscroll_timeline.is_playing () == false && is_autoscrolling)
        {
          autoscroll_timeline.start ();
        }
      else if (autoscroll_timeline.is_playing () && is_autoscrolling == false)
        {
          autoscroll_timeline.stop ();
        }
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
      if (is_scrolling)
        {
          passthrough_button_press_event (event);
        }
      button_down = true;
      if (get_model_index_at_y_pos (event.button.y, true) < 0)
        can_scroll = false;
      else
        can_scroll = true;

      previous_y_position = event.button.y;
      previous_y_time = event.button.time;

      this.get_stage ().button_release_event.connect (this.on_button_release_event);

      return false;
    }

    private bool on_button_release_event (Clutter.Event event)
    {
      if (event.button.button != 1)
        {
          // not a left click
          return false;
        }

      button_down = false;
      this.get_stage ().button_release_event.disconnect (this.on_button_release_event);
      Unity.global_shell.remove_fullscreen_request (this);
      Clutter.ungrab_pointer ();

      if (is_scrolling)
        {
          //passthrough_button_release_event (event);
          foreach (ScrollerChild child in model)
            {
              child.grabbed_push = 0;
            }
          is_scrolling = false;
          Unity.Launcher.disable_quicklists = false;
          Clutter.ungrab_pointer ();
          get_stage ().motion_event.disconnect (on_motion_event);
          if (scroll_position > 0 || scroll_position < -(get_total_children_height () - get_available_height ()))
            {
              current_phase = ScrollerPhase.SETTLING;
              settle_position = get_aligned_settle_position ();
            }
          else
            {
              current_phase = ScrollerPhase.FLUNG;
            }

          disable_child_events = true;
          fling_timeline.start ();
        }

      MenuManager manager = MenuManager.get_default ();
      manager.popdown_current_menu ();

      return false;
    }


    uint queue_contract_launcher = 0;
    private bool on_enter_event (Clutter.Event event)
    {
      if (queue_contract_launcher != 0)
        {
          Source.remove (queue_contract_launcher);
          queue_contract_launcher = 0;
        }

      if (attached_menu is QuicklistController)
        {
          attached_menu.notify["status"].disconnect (on_menu_close);
          attached_menu = null;
        }

      expand_launcher (event.crossing.y);
      return false;
    }

    private bool on_queue_contract_launcher ()
    {
      if (queue_contract_launcher != 0)
        {
          current_phase = ScrollerPhase.NONE;
          contract_launcher ();
        }
      queue_contract_launcher = 0;
      return false;
    }

    private bool do_queue_contract_launcher ()
    {
      queue_contract_launcher = Timeout.add (250, on_queue_contract_launcher);
      return false;
    }

    QuicklistController? attached_menu = null;
    private void on_menu_close ()
    {
      if (attached_menu is QuicklistController)
        {
          if (attached_menu.state != QuicklistControllerState.MENU)
            {
              if (last_known_pointer_x > get_width ())
                do_queue_contract_launcher ();

              attached_menu.notify["status"].disconnect (on_menu_close);
              attached_menu = null;
            }
        }
    }

    private bool on_leave_event (Clutter.Event event)
    {
      last_known_pointer_x = 200;
      var drag_controller = Drag.Controller.get_default ();
      if (drag_controller.is_dragging) return false;
      if (is_scrolling) return false;

      // if a menu is open, don't fold the launcher, wait until its closed (if ever)
      QuicklistController? menu = QuicklistController.get_current_menu ();
      if (menu is QuicklistController)
        {
          if (menu.state == QuicklistControllerState.MENU)
            {
              attached_menu = menu;
              attached_menu.notify["state"].connect (on_menu_close);
              return false;
            }
          }

      do_queue_contract_launcher ();

      if (last_picked_actor is Clutter.Actor &&
          last_picked_actor != this)
        {
          last_picked_actor.do_event (event, false);
          last_picked_actor = null;
        }
      return false;
    }

    float autoscroll_mouse_pos_cache = 0.0f;
    private bool on_autoscroll_motion_check (float y)
    {
      if (get_total_children_height () < get_available_height () || is_scrolling)
        {
          is_autoscrolling = false;
        }
      else
        {
          //check for autoscroll events
          float pos_x, pos_y;
          get_transformed_position (out pos_x, out pos_y);
          float transformed_y = y - pos_y;

          autoscroll_mouse_pos_cache = transformed_y;
          if (transformed_y > (get_height ()/2))
            {
              autoscroll_direction = -1;
              autoscroll_mouse_pos_cache -= get_height ();
            }
          else
            {
              autoscroll_direction = 1;
            }
          if (transformed_y < 12 || transformed_y > (get_height () - 12))
            is_autoscrolling = true;
          else
            is_autoscrolling = false;
        }
      return false;
    }

    private void on_drag_motion_event (Unity.Drag.Model model, float x, float y)
    {
      on_autoscroll_motion_check (y);
    }

    private bool on_motion_event (Clutter.Event event)
    {
      on_autoscroll_motion_check (event.motion.y);

      var drag_controller = Drag.Controller.get_default ();
      if (drag_controller.is_dragging)
      {
        // we are dragging from somewhere else, ignore motion events
        return false;
      }
      last_motion_event_time = event.motion.time;

      if (button_down && is_scrolling == false &&
          view_type != ScrollerViewType.CONTRACTED &&
          can_scroll)
        {
          /* we have a left button down, but we aren't dragging yet, we need to
           * monitor how far away we have dragged from the original click, once
           * we get far enough away we can start scrolling.
           */
          //var diff = event.motion.y - previous_y_position;
          is_scrolling = true;
          Unity.Launcher.disable_quicklists = true;
          Unity.global_shell.add_fullscreen_request (this);
          Clutter.grab_pointer (this);
          get_stage ().motion_event.connect (on_motion_event);
        }

      if (is_scrolling)
        {
          /* Disable any animations on the children */
          //disable_animations_on_children (event);

          /* we need to compare the event y position from this event to the
           * previous event. once we have that we can compute a velocity based
           * on how long it was since the previous event
           */
          passthrough_motion_event (event);

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

    private bool on_scroll_event (Clutter.Event event)
    {
      // got a mouse wheel scroll
      float modifier = 0.0f;
      if (event.scroll.direction == Clutter.ScrollDirection.UP)
        modifier = 1.0f;
      else if (event.scroll.direction == Clutter.ScrollDirection.DOWN)
        modifier = -1.0f;

      if (modifier != 0.0f)
        {
          float speed = ((48 + spacing) * 3) * 6.0f;
          scroll_speed += speed * modifier;
          current_phase = ScrollerPhase.FLUNG;
          fling_timeline.start ();
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
          if (fling_timeout_source != 0 && current_phase != ScrollerPhase.NONE)
            Source.remove (fling_timeout_source);

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
              {
                timeline.stop ();
                scroll_speed = 0.0f;
                is_animating = false;
                disable_child_events = false;
              }
              break;
            default:
              assert_not_reached ();
          }
        }

      if (current_phase == ScrollerPhase.NONE)
        cache.update_texture_cache ();
      else
        cache.invalidate_texture_cache ();

      Clutter.Event motion_event =  { 0 };
      motion_event.type = Clutter.EventType.MOTION;
      motion_event.motion.x = last_known_x;
      motion_event.motion.y = last_known_y;
      motion_event.motion.stage = get_stage () as Clutter.Stage;
      motion_event.motion.flags = Clutter.EventFlags.FLAG_SYNTHETIC;
      passthrough_motion_event (motion_event);

      stored_delta = delta;
    }

    private void do_anim_settle (Clutter.Timeline timeline, int msecs)
    {
      var distance = settle_position - scroll_position;
      move_scroll_position (distance * 0.2f, false, 60.0f);
      if (Math.fabs (distance) < 1 )
        {
          move_scroll_position (distance);
          current_phase = ScrollerPhase.NONE;
        }

    }

    uint fling_timeout_source = 0;
    private void do_anim_fling (Clutter.Timeline timeline, int msecs)
    {
      scroll_speed *= friction; // slow our speed

      // we devide by 60 because get 60 ticks a second
      float scroll_move_amount = scroll_speed / 60.0f;
      move_scroll_position (scroll_move_amount, false, 60.0f);

      //after a fling, we have to figure out if we want to change our
      // scroller phase or not


      if (Math.fabsf (scroll_move_amount) < 1.0)
        {
          current_phase = ScrollerPhase.NONE;
          fling_timeout_source = GLib.Timeout.add (300, () =>
            {
              current_phase = ScrollerPhase.SETTLING;
              settle_position = get_aligned_settle_position ();
              fling_timeline.start ();
              return false;
            });
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
      else if (get_total_children_height () < get_available_height ())
        {
          final_position = 0;
        }
      else if (-scroll_position > total_child_height - height - padding.top - padding.bottom)
        {
          // position on the final child
          final_position = -(get_total_children_height () - get_available_height ());
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
      if (get_total_children_height () < get_available_height ())
        {
          order_children_expanded (immediate);
        }
      else
        {
          switch (view_type)
            {
              case ScrollerViewType.CONTRACTED:
                order_children_contracted (immediate);
                break;

              case ScrollerViewType.EXPANDED:
                order_children_expanded (immediate);
                break;

              default:
                assert_not_reached ();
            }
        }
      queue_relayout ();
    }

    private void change_child_position_rotation (ScrollerChild child,
                                                 float position, float rotation,
                                                 bool immediate = false)
    {
      if (immediate || global_shell.is_starting)
        {
          child.position = position;
          child.force_rotation_jump (rotation);
        }
      else
        {
          bool do_new_position = true;
          if (child.get_animation () is Clutter.Animation)
            {
              //GLib.Value value = GLib.Value (GLib.Type.from_name ("string"));
              GLib.Value value = Value (typeof (float));
              Clutter.Interval interval = child.get_animation ().get_interval ("position");
              if (interval is Clutter.Interval)
                interval.get_final_value (value);
              if (value.get_float () != position)
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

          child.rotation = rotation;

          if (do_new_position)
            {
              if (view_type == ScrollerViewType.CONTRACTED)
                child.animate (Clutter.AnimationMode.EASE_IN_OUT_QUAD, 300,
                               "position", position);
              else
                child.animate (Clutter.AnimationMode.EASE_OUT_QUINT, 300,
                               "position", position);
            }
        }
    }

    private void order_children_expanded (bool immediate = false)
    {
      // figures out the position of each child based on its order in the model
      float h = 0.0f;
      float min_height, nat_height;
      if (!(draw_ftb is Gee.ArrayList))
        draw_ftb = new Gee.ArrayList<ScrollerChild> ();

      if (!(draw_ftb is Gee.ArrayList))
        draw_btf = new Gee.ArrayList<ScrollerChild> ();

      int index = 0;
      foreach (ScrollerChild child in model)
      {

        if (index == drag_indicator_index && drag_indicator_active)
          {
            if (drag_indicator_space)
              {
                child.get_preferred_height (get_width (), out min_height, out nat_height);
                h += nat_height + spacing;
              }
            else
              {
                h += 2 + spacing;
              }
          }
        else
          {
            child.get_preferred_height (get_width (), out min_height, out nat_height);
            change_child_position_rotation (child, h + scroll_position, 0.0f, immediate);

            h += nat_height + spacing;
          }
        index += 1;
      }
    }

    private void order_children_contracted (bool immediate = false)
    {
      float h = 0.0f;
      float min_height, nat_height;
      int num_launchers = 0;
      //get the total size of the children in a flat state
      float total_child_height = get_total_children_height ();
      int actual_model_size = model.size;
      foreach (ScrollerChild child in model)
        if (child.do_not_render) actual_model_size -= 1;

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

              contracted_space = ((actual_model_size - num_launchers) * (8 + spacing));

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
          num_launchers = actual_model_size;
        }

      int num_children_handled = 0;
      int index_start_flat, index_end_flat = 0;

      if (focused_launcher < actual_model_size - (num_launchers -(num_launchers / 2)))
        {
          index_start_flat = int.max (0, focused_launcher - (num_launchers / 2));
          index_end_flat = index_start_flat + num_launchers;
        }
      else
        {
          index_end_flat = actual_model_size;
          index_start_flat = index_end_flat - num_launchers;
        }
      draw_ftb = new Gee.ArrayList<ScrollerChild> ();
      draw_btf = new Gee.ArrayList<ScrollerChild> ();

      for (int index = 0; index < model.size; index++)
        {
          ScrollerChild child = model[index];
          child.get_preferred_height (get_width (), out min_height, out nat_height);
          if (index >= index_start_flat && index < index_end_flat)
            {
              change_child_position_rotation (child, h, 0.0f, immediate);
              h += 48 + spacing;
              num_children_handled++;

              if (index == index_start_flat)
                draw_ftb.add (child);
              else
                draw_btf.add (child);
            }
          else
            {
              // contracted launcher
              if (index == index_end_flat) h -= nat_height * 0.3333f - spacing;//spacing * 2;
              float rotation = 0.0f;
              float position = 0.0f;
              if (num_children_handled < index_start_flat)
                {
                  if (num_children_handled == index_start_flat - 1)
                    {
                      rotation = -contract_icon_partial_degrees;
                      h += spacing;
                    }
                  else
                    {
                      rotation = -contract_icon_degrees;
                    }
                  position = h;
                  draw_ftb.add (child);
                }
              else
                {
                  position = h;
                  if (index == index_end_flat)
                    {
                      rotation = contract_icon_partial_degrees;
                      h += spacing;
                    }
                  else
                    {
                      rotation = contract_icon_degrees;
                    }
                  draw_btf.add (child);
                }

              change_child_position_rotation (child, position, rotation, immediate);
              h += 8 + spacing;
              num_children_handled++;

              if (index +1 == index_start_flat) h += 30;
            }
        }
    }


    private float get_total_children_height ()
    {
      float h = 0.0f;
      float min_height, nat_height;
      foreach (ScrollerChild child in model)
        {
          if (child.do_not_render) continue;
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
      Clutter.ActorBox temp_child_box = Clutter.ActorBox ();
      float current_width = padding.left;
      float available_height = box.get_height () - padding.bottom;
      float available_width = box.get_width () - padding.right;

      total_child_height = 0.0f;
      int index = 0;

      if (drag_indicator_active)
        drag_indicator_position = model[drag_indicator_index].position + padding.top;

      foreach (ScrollerChild child in model)
        {

          float child_height, child_width, natural, min;

          child.get_preferred_width (available_height, out min, out natural);
          child_width = Math.fmaxf (min, Math.fminf (natural, available_width));

          child.get_preferred_height (child_width, out min, out natural);
          child_height = Math.fmaxf (min, Math.fminf (natural, available_height));

          child_box.x1 = current_width;
          child_box.x2 = child_box.x1 + child_width;//box.get_width () - padding.right;
          child_box.y1 = child.position + padding.top;
          child_box.y2 = child_box.y1 + child_height;

          if (!child.do_not_render) ;
            child.allocate (child_box, flags);

          total_child_height += child_height + spacing;

          if (index >= 0 && index <= 9)
          {
            Clutter.CairoTexture? keyboard_indicator = null;
            keyboard_indicator = keyboard_indicators[(int)index];

            if (keyboard_indicator is Clutter.Actor)
              {
                uint surface_width, surface_height;
                keyboard_indicator.get_surface_size (out surface_width, out surface_height);
                child_box.x1 = box.get_width () - padding.right - surface_width - 6;
                child_box.x2 = child_box.x1 + keyboard_indicator.get_width ();
                child_box.y1 = child.position + padding.top + ((child_box.get_height ()*0.5f) - (surface_height*0.5f));
                child_box.y2 = child_box.y1 + keyboard_indicator.get_height ();
                keyboard_indicator.allocate (child_box, flags);
              }

          index += 1;
        }
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
      foreach (ScrollerChild child in model)
        {
          if (child is ScrollerChild && child.opacity > 0 && !child.do_not_render)
            {
              (child as ScrollerChild).paint ();
            }
        }

      foreach (ScrollerChild child in child_refs)
        {
          if (child.do_not_render) continue;
          child.paint ();
        }
    }


    public override void paint ()
    {
      bgtex.paint ();

      if (drag_indicator_active)
        {
          Cogl.set_source_color4f (1.0f, 1.0f, 1.0f,
                                   drag_indicator_opacity);

          Cogl.rectangle (0, drag_indicator_position,
                          get_width (),
                          drag_indicator_position + 2);

        }

      for (int index = draw_btf.size-1; index >= 0; index--)
        {
          ScrollerChild child = draw_btf[index];
          if (child is ScrollerChild && child.opacity > 0 && !child.do_not_render)
            {
              (child as ScrollerChild).paint ();
            }
        }

      foreach (ScrollerChild child in draw_ftb)
        {
          if (child is ScrollerChild && child.opacity > 0 && !child.do_not_render)
            {
              (child as ScrollerChild).paint ();
            }
        }

      foreach (ScrollerChild child in model)
        {
          if ((child in draw_ftb) || (child in draw_btf))
            continue;

          if (child is ScrollerChild && child.opacity > 0)
            {
              (child as ScrollerChild).paint ();
            }
        }


      foreach (ScrollerChild child in child_refs)
        {
          if (child.do_not_render) continue;
          child.paint ();
        }

      foreach (Clutter.CairoTexture kb_ind in keyboard_indicators)
        {
          kb_ind.paint ();
        }

      top_shadow.paint ();
    }

    public override void map ()
    {
      base.map ();
      bgtex.map ();
      top_shadow.map ();

      foreach (Clutter.CairoTexture kb_ind in keyboard_indicators)
        {
          kb_ind.map ();
        }

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

      foreach (Clutter.CairoTexture kb_ind in keyboard_indicators)
        {
          kb_ind.paint ();
        }

      foreach (ScrollerChild child in model)
        {
          child.unmap ();
        }
    }
  }

}
