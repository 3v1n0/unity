/* -*- Mode: vala; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/*
 * Copyright (C) 2009-2010 Canonical Ltd
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
  class ScrollerController : Object
  {
    public ScrollerModel model {get; construct;}
    public ScrollerView view {get; construct;}
    private Gee.ArrayList<ScrollerChildController> childcontrollers;

    /* constants */
    private const uint DRAG_SAFE_ZONE = 200;

    private Bamf.Matcher matcher;

    /* favourites */
    private Unity.Favorites favorites;

    public ScrollerController (ScrollerModel _model, ScrollerView _view)
    {
      Object (model:_model, view: _view);
    }

    construct
    {
      model.order_changed.connect (model_order_changed);
      childcontrollers = new Gee.ArrayList<ScrollerChildController> ();

      matcher = Bamf.Matcher.get_default ();
      favorites = Unity.Favorites.get_default ();
      favorites.favorite_added.connect (on_favorite_added);
      matcher.view_opened.connect (handle_bamf_view_opened);

      build_favorites ();

      foreach (Object object in (List<Bamf.View>)(matcher.get_running_applications ()))
        {
          if (object is Bamf.View)
            {
              handle_bamf_view_opened (object);
            }
          else
            {
              error ("Bamf returned a strange object");
            }
        }

      // hook up to the drag controller
      var drag_controller = Drag.Controller.get_default ();
      drag_controller.drag_start.connect (on_unity_drag_start);

      Unity.global_shell.notify["super-key-active"].connect (on_super_key_active);
      Unity.global_shell.super_key_modifier_release.connect (on_super_key_modifier_release);
    }

    private void on_super_key_modifier_release (uint keycode)
    {
      if (!Unity.global_shell.super_key_active) return;
      int index = (int)keycode - 10;
      index = int.min (index, model.size - 1);
      if (index < 0 || index > 9) return;

      Unity.global_shell.super_key_active = false;

      var childcontroller = get_controller_for_view (model[index]);
      if (childcontroller != null)
        {
          childcontroller.activate ();
        }
      else
        {
          print ("---=== childcontroller is NULL :( ===---\n");
        }
    }

    uint super_key_source = 0;
    private void on_super_key_active ()
    {
      if (Unity.global_shell.super_key_active && super_key_source == 0)
        super_key_source = Timeout.add (300, () => {
          view.enable_keyboard_selection_mode (Unity.global_shell.super_key_active);
          return false;
        });
      else
        {
          if (super_key_source != 0)
            {
              Source.remove (super_key_source);
              super_key_source = 0;
            }
          view.enable_keyboard_selection_mode (false);
        }
    }

    private void handle_bamf_view_opened (Object object)
      requires (object is Bamf.View)
    {
      if (object is Bamf.Application)
        {
          Bamf.Application app = object as Bamf.Application;
          // need to hook up to its visible changed signals

          string desktop_file = app.get_desktop_file ();

          ScrollerChildController controller = null;
          if (desktop_file != null && desktop_file != "")
            {
              controller = find_controller_by_desktop_file (desktop_file);
            }

          if (controller is ApplicationController)
            {
              (controller as ApplicationController).attach_application (app);
            }
          else
            {
              ScrollerChild child = new ScrollerChild ();
              child.group_type = ScrollerChild.GroupType.APPLICATION;
              controller = new ApplicationController (null, child);
              (controller as ApplicationController).attach_application (app);
              if (app.user_visible ())
                model.add (child);

              childcontrollers.add (controller);
              controller.request_removal.connect (on_scroller_controller_closed);
              controller.notify["hide"].connect (() => {

                // weird logic here i know, basically more if's than needed because
                // i don't want to access the controller as a subclass until
                // i know it is of that subclass type.
                if (controller.hide && controller.child in model)
                  {
                    if (controller is ApplicationController)
                      {
                        if ((controller as ApplicationController).is_favorite == false)
                          model.remove (controller.child);
                      }
                    else
                      {
                        model.remove (controller.child);
                      }
                  }
                else if (!controller.hide && (controller.child in model) == false)
                  model.add (controller.child);
              });
            }
        }
    }

    private void on_scroller_controller_closed (ScrollerChildController controller)
    {
      if (controller is ApplicationController)
        {
          if (!(controller as ApplicationController).is_sticky ())
            {
              model.remove (controller.child);
              childcontrollers.remove (controller);
            }
        }
    }

    private static int compare_prioritys (ScrollerChild a, ScrollerChild b)
    {
      float prioritya = -1.0f, priorityb = -1.0f;
      ScrollerChildController? childcontroller = null;

      try
        {
          childcontroller = a.controller;
          if (childcontroller is ApplicationController)
            prioritya = (childcontroller as ApplicationController).get_priority ();
        }
      catch (AppTypeError e)
        {
          prioritya = 10000.0f;
        }

      try
        {
          childcontroller = b.controller;
          if (childcontroller is ApplicationController)
            priorityb = (childcontroller as ApplicationController).get_priority ();
        }
      catch (AppTypeError e)
        {
          priorityb = 10000.0f;
        }

      if (prioritya < priorityb) return -1;
      if (prioritya > priorityb) return  1;
      return 0;
    }

    private void build_favorites ()
    {
      model.order_changed.disconnect (model_order_changed);
      // build a list of favourites, so we need a list of favourites
      string[] bamf_favorites_list = {};

      foreach (string uid in favorites.get_favorites ())
        {
          var type = favorites.get_string (uid, "type");
          if (type != "application")
            continue;

          string desktop_file = favorites.get_string (uid, "desktop_file");
          if (!FileUtils.test (desktop_file, FileTest.EXISTS))
            {
              // no desktop file for this favorite or it does not exist
              continue;
            }

          bamf_favorites_list += desktop_file;

          ApplicationController controller = find_controller_by_desktop_file (desktop_file);
          if (!(controller is ScrollerChildController))
            {
              ScrollerChild child = new ScrollerChild ();
              controller = new ApplicationController (desktop_file, child);
              model.add (child);
              childcontrollers.add (controller);
              controller.request_removal.connect (on_scroller_controller_closed);
            }
        }

      // need to sort the list now
      model.sort ((CompareFunc)compare_prioritys);
      model.order_changed.connect (model_order_changed);

      // upload to bamf
      matcher.register_favorites (bamf_favorites_list);
    }

    private void on_favorite_added (string uid)
    {
      var desktop_file = favorites.get_string (uid, "desktop_file");
      if (!FileUtils.test (desktop_file, FileTest.EXISTS))
        {
          // no desktop file for this favorite or it does not exist
          return;
        }

      ApplicationController controller = find_controller_by_desktop_file (desktop_file);
      if (!(controller is ScrollerChildController))
        {
          ScrollerChild child = new ScrollerChild ();
          controller = new ApplicationController (desktop_file, child);
          model.add (child);
          childcontrollers.add (controller);
          controller.request_removal.connect (on_scroller_controller_closed);
        }
    }

    private ApplicationController? find_controller_by_desktop_file (string desktop_file)
    {
      foreach (ScrollerChildController controller in childcontrollers)
        {
          if (controller is ApplicationController)
            {
              if ((controller as ApplicationController).desktop_file == desktop_file)
                {
                  return controller as ApplicationController;
                }
            }
        }
      return null;
    }

    /*
     * drag controller connections
     */
    private void on_unity_drag_start (Drag.Model drag_model)
    {
      var drag_controller = Drag.Controller.get_default ();

      drag_controller.drag_motion.connect (on_unity_drag_motion);
      drag_controller.drag_drop.connect (on_unity_drag_drop);

      // find the model and hide it :)
      if (drag_controller.get_drag_model () is ScrollerChildController)
      {
        ScrollerChild child = (drag_controller.get_drag_model () as ScrollerChildController).child;
        child.opacity = 0;
        child.do_not_render = true;
        view.drag_indicator_active = true;
        view.drag_indicator_space = true;
        view.do_queue_redraw ();
      }
    }

    float last_drag_x = 0.0f;
    float last_drag_y = 0.0f;
    private void on_unity_drag_motion (Drag.Model drag_model, float x, float y)
    {
      if (x == last_drag_x && y == last_drag_y)
        return;

      last_drag_x = x;
      last_drag_y = y;

      var drag_controller = Drag.Controller.get_default ();
      // check to see if the data matches any of our children
      if (!(drag_controller.get_drag_model () is ScrollerChildController))
      {
        return;
      }
      ScrollerChild retcont = (drag_controller.get_drag_model () as ScrollerChildController).child;

      if (x > view.get_width () + DRAG_SAFE_ZONE &&
          retcont.group_type != ScrollerChild.GroupType.PLACE &&
          retcont.group_type != ScrollerChild.GroupType.SYSTEM)
        {
          // we need to remove this child from the model, its been dragged out
          model.remove (retcont);
          view.drag_indicator_active = false;
          if (retcont is ScrollerChild)
            {
              if (retcont.enable_close_state == false);
                {
                  retcont.enable_close_state = true;
                }
            }
        }
      else
        {
          if (retcont is ScrollerChild)
            {

              if (retcont.enable_close_state == true)
                {
                  retcont.enable_close_state = false;
                }
            }

          if (x < view.get_width ())
            {
              if (view.drag_indicator_space != true)
                view.drag_indicator_space = true;
            }
          else if (x > view.get_width () && x < view.get_width () + DRAG_SAFE_ZONE)
            {
              if (view.drag_indicator_space != false)
                view.drag_indicator_space = false;

              if (view.drag_indicator_active != true)
                view.drag_indicator_active = true;
            }

          // if the actor is not in the model, add it. because its now in there!
          // find the index at this position

          int model_index = view.get_model_index_at_y_pos_no_anim (y - 24, true);
          if (model_index < 0) return;

          //we have to check to see if we would still be over the index
          //if it was done animating
          if (retcont in model)
            model.move (retcont, int.max (model_index, 0));
          else
            model.insert (retcont, int.max (model_index, 0));

          if (model_index != view.drag_indicator_index)
            {
              view.drag_indicator_index = model_index;
              view.do_queue_redraw ();
            }
        }
    }

    private void on_unity_drag_drop (Drag.Model drag_model, float x, float y)
    {
      var drag_controller = Drag.Controller.get_default ();
      // check to see if the data matches any of our children
      if (!(drag_controller.get_drag_model () is ScrollerChildController))
      {
        view.cache.update_texture_cache ();
        return;
      }
      view.drag_indicator_active = false;
      ScrollerChildController model_controller = drag_controller.get_drag_model () as ScrollerChildController;
      ScrollerChild retcont = model_controller.child;

      if (retcont.group_type == ScrollerChild.GroupType.PLACE ||
          retcont.group_type == ScrollerChild.GroupType.SYSTEM)
        {
          ; /* Do nothing */
        }
      else if (x > view.get_width () + DRAG_SAFE_ZONE)
        {
          // it was dropped outside of the launcher.. oh well, obliterate it.
          if (retcont.controller is ApplicationController)
            {
              (retcont.controller as ApplicationController).set_sticky (false);
              (retcont.controller as ApplicationController).close_windows ();
            }
          if (retcont in model ||
              retcont.group_type == ScrollerChild.GroupType.DEVICE)
            {
              retcont.drag_removed ();
              model.remove (retcont);
            }
          if (model_controller is ApplicationController)
            {
              (model_controller as ApplicationController).set_sticky (false);
            }

          if (model_controller in childcontrollers)
            childcontrollers.remove (model_controller);
        }
      else
        {
          if (model_controller is ApplicationController)
            (model_controller as ApplicationController).set_sticky ();
        }
      // if it was dropped inside of the launcher, its allready been added
      retcont.opacity = 255;
      retcont.do_not_render = false;
      // disconnect our drag controller signals
      drag_controller.drag_motion.disconnect (this.on_unity_drag_motion);
      drag_controller.drag_drop.disconnect (this.on_unity_drag_drop);

      view.cache.update_texture_cache ();
    }

    private ScrollerChildController? get_controller_for_view (ScrollerChild childview)
    {
      print ("---=== get_controller_for_view(): size= %d ===---\n",
             childcontrollers.size);
      foreach (ScrollerChildController childcontroller in childcontrollers)
        {
          if (childcontroller.child == childview)
            return childcontroller;
        }
      return null;
    }

    private void model_order_changed ()
    {
      float index = 1.0f;
      foreach (ScrollerChild childview in model)
      {
        ScrollerChildController? childcontroller = get_controller_for_view (childview);
        if (childcontroller is ApplicationController)
          (childcontroller as ApplicationController).set_priority (index);
        index += 1.0f;
      }
    }

  }
}
