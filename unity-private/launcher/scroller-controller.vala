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

namespace Unity.Launcher
{
  class ScrollerController : Object
  {
    public ScrollerModel model {get; construct;}
    public ScrollerView view {get; construct;}
    private Gee.ArrayList<ScrollerChildController> childcontrollers;

    /* constants */
    private const uint DRAG_SAFE_ZONE = 300;

    /* liblauncher support */
    private LibLauncher.Appman appman;
    private LibLauncher.Session session;

    public ScrollerController (ScrollerModel _model, ScrollerView _view)
    {
      Object (model:_model, view: _view);
    }

    construct
    {
      childcontrollers = new Gee.ArrayList<ScrollerChildController> ();
      appman = LibLauncher.Appman.get_default ();
      session = LibLauncher.Session.get_default ();

      session.application_opened.connect (handle_session_application);
      build_favorites ();

      // hook up to the drag controller
      var drag_controller = Drag.Controller.get_default ();
      drag_controller.drag_start.connect (on_unity_drag_start);
    }

    private void handle_session_application (LibLauncher.Application app)
    {
      string desktop_file = app.get_desktop_file ();
      if (desktop_file != null)
        {
          var controller = find_controller_by_desktop_file (desktop_file);
          if (controller is ApplicationController)
            {
              // already in our model, just attach the app
              controller.attach_application (app);
            }
          else
            {
              LauncherChild child = new LauncherChild ();
              controller = new ApplicationController (desktop_file, child);
              controller.attach_application (app);
              model.add (child);
              childcontrollers.add (controller);
            }
        }
    }

    private void build_favorites ()
    {
      debug ("building favorites");
      var favorites = LibLauncher.Favorites.get_default ();

      unowned SList<string> favorite_list = favorites.get_favorites();
      foreach (weak string uid in favorite_list)
        {
          debug (@"adding favorite $uid");
          // we only want favorite *applications* for the moment
          var type = favorites.get_string(uid, "type");
          if (type != "application")
              continue;

          string? desktop_file = favorites.get_string(uid, "desktop_file");
          assert (desktop_file != "");
          if (!FileUtils.test (desktop_file, FileTest.EXISTS))
            {
              // we don't have a desktop file that exists, remove it from the
              // favourites
              favorites.remove_favorite (uid);
            }
          else
            {
              debug (@"favorite desktop file: $desktop_file");
              LauncherChild child = new LauncherChild ();
              ApplicationController controller = new ApplicationController (desktop_file, child);
              model.add (child);
              childcontrollers.add (controller);
            }
          debug ("number of items in model: %i", model.size);
        }
    }

    private bool desktop_file_is_favorite (string desktop_file)
    {
      var favorites = LibLauncher.Favorites.get_default ();

      unowned SList<string> favorite_list = favorites.get_favorites();
      foreach (weak string uid in favorite_list)
        {
          var type = favorites.get_string(uid, "type");
          if (type != "application")
              continue;

          string fav_desktop_file = favorites.get_string(uid, "desktop_file");
          if (desktop_file == fav_desktop_file)
            {
              return true;
            }
        }

      return false;
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
      string data = drag_model.get_drag_data ();

      drag_controller.drag_motion.connect (on_unity_drag_motion);
      drag_controller.drag_drop.connect (on_unity_drag_drop);

      // find the model and hide it :)
      if (drag_controller.get_drag_model () is ScrollerChildController)
      {
        ScrollerChild child = (drag_controller.get_drag_model () as ScrollerChildController).child;
        debug (@"our model is $child");
        //child.opacity = 0;
        //child.hide ();
        view.do_queue_redraw ();
        debug (@"our model is $child");
      }
    }

    private void on_unity_drag_motion (Drag.Model drag_model, float x, float y)
    {
      var drag_controller = Drag.Controller.get_default ();
      //find child
      string data = drag_model.get_drag_data ();
      // check to see if the data matches any of our children
      if (!(drag_controller.get_drag_model () is ScrollerChildController))
      {
        return;
      }
      ScrollerChild retcont = (drag_controller.get_drag_model () as ScrollerChildController).child;

      if (x > view.get_width () + DRAG_SAFE_ZONE)
        {
          // we need to remove this child from the model, its been dragged out
          model.remove (retcont);
        }
      else
        {
          // if the actor is not in the model, add it. because its now in there!
          if (retcont in model)
            {
              model.remove (retcont); // we have to remove it before re-adding
            }

          // find the index at this position
          int model_index = view.get_model_index_at_y_pos (y);
          model.insert (retcont, model_index);
/*
          retcont.set_opacity (255);
          retcont.show ();
*/

          view.do_queue_redraw ();
        }
    }

    private void on_unity_drag_drop (Drag.Model drag_model, float x, float y)
    {
      var drag_controller = Drag.Controller.get_default ();
      string data = drag_model.get_drag_data ();
      // check to see if the data matches any of our children
      if (!(drag_controller.get_drag_model () is ScrollerChildController))
      {
        return;
      }
      ScrollerChildController model_controller = drag_controller.get_drag_model () as ScrollerChildController;
      ScrollerChild retcont = model_controller.child;

      if (x > view.get_width ())
        {
          // it was dropped outside of the launcher.. oh well, obliterate it.
          if (retcont in model)
            model.remove (retcont);

          if (model_controller in childcontrollers)
            childcontrollers.remove (model_controller);
        }
      // if it was dropped inside of the launcher, its allready been added

      // disconnect our drag controller signals
      drag_controller.drag_motion.disconnect (this.on_unity_drag_motion);
      drag_controller.drag_drop.disconnect (this.on_unity_drag_drop);
    }
  }
}
