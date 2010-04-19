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
    public ScrollerModel model {get; set;}
    private Gee.ArrayList<ScrollerChildController> childcontrollers;

    /* liblauncher support */
    private LibLauncher.Appman appman;
    private LibLauncher.Session session;

    public ScrollerController (ScrollerModel model)
    {
      Object (model:model);
    }

    construct
    {
      childcontrollers = new Gee.ArrayList<ScrollerChildController> ();
      appman = LibLauncher.Appman.get_default ();
      session = LibLauncher.Session.get_default ();

      session.application_opened.connect (handle_session_application);
    }

    private void handle_session_application (LibLauncher.Application app)
    {
      string desktop_file = app.get_desktop_file ();
      if (desktop_file != null)
        {
          if (desktop_file_is_favorite (desktop_file))
            {
              var controller = find_controller_by_desktop_file (desktop_file);
              if (controller is ScrollerChildController)
                {
                  controller.desktop_file = desktop_file;
                }
            }
          else
            {
              LauncherChild child = new LauncherChild ();
              ApplicationController controller = new ApplicationController (desktop_file);
              model.add (child);
              childcontrollers.add (controller);
            }
        }
    }

    private void build_favorites ()
    {
      var favorites = LibLauncher.Favorites.get_default ();

      unowned SList<string> favorite_list = favorites.get_favorites();
      foreach (weak string uid in favorite_list)
        {
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
              LauncherChild child = new LauncherChild ();
              ApplicationController controller = new ApplicationController (desktop_file);
              model.add (child);
              childcontrollers.add (controller);
            }
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

    private ScrollerChildController? find_controller_by_desktop_file (string desktop_file)
    {
      foreach (ScrollerChildController controller in childcontrollers)
        {
          if (controller.desktop_file == desktop_file)
            {
              return controller;
            }
        }
      return null;
    }
  }
}
