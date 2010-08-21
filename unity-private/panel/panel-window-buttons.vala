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
 * Authored by Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */

namespace Unity.Panel
{
  public class WindowButtons : Ctk.Box
  {
    private static const string FORMAT = "<b>%s</b>";
    private Ctk.Text     appname;
    private WindowButton close;
    private WindowButton minimize;
    private WindowButton unmaximize;

    private unowned Bamf.Matcher matcher;
    private AppInfoManager       appinfo;

    private uint32 last_xid = 0;

    private unowned Bamf.View _last_view = null;

    public WindowButtons ()
    {
      Object (orientation:Ctk.Orientation.HORIZONTAL,
              spacing:2,
              homogeneous:false);
    }

    construct
    {
      appname = new Ctk.Text (" ");
      appname.use_markup = true;
      appname.max_length = 9;
      pack (appname, true, true);

      close = new WindowButton ("close");
      pack (close, false, false);
      close.clicked.connect (() => {
        if (last_xid > 0)
          global_shell.do_window_action (last_xid, WindowAction.CLOSE);
      });

      minimize = new WindowButton ("minimize");
      pack (minimize, false, false);
      minimize.clicked.connect (() => {
        if (last_xid > 0)
          global_shell.do_window_action (last_xid, WindowAction.MINIMIZE);
      });

      unmaximize = new WindowButton ("unmaximize");
      pack (unmaximize, false, false);
      unmaximize.clicked.connect (() => {
        if (last_xid > 0)
          global_shell.do_window_action (last_xid, WindowAction.UNMAXIMIZE);
      });

      /* Grab AppInfoManager for the desktop file lookups */
      appinfo = AppInfoManager.get_instance ();
      
      /* Grab matcher that gives us state */
      matcher = Bamf.Matcher.get_default ();
      matcher.active_window_changed.connect (on_active_window_changed);

      /* Shell will let us know when something changes with the active window */
      global_shell.active_window_state_changed.connect (() => {
        Timeout.add (0, () => {
          unowned Bamf.Window? win = matcher.get_active_window ();
          on_active_window_changed (null, win as GLib.Object);

          return false;
        });
      });

      Idle.add (() => {
        unowned Bamf.Window? win = matcher.get_active_window ();
        on_active_window_changed (null, win as GLib.Object);

        return false;
      });
    }

    private void on_active_window_changed (GLib.Object? object,
                                           GLib.Object? object1)
    {
      unowned Bamf.View? new_view = object1 as Bamf.View;

      appname.set_markup ("");
      appname.hide ();
      close.hide ();
      minimize.hide ();
      unmaximize.hide ();
      last_xid = 0;

      if (object is Bamf.View)
        {
          if (object == _last_view)
            {
              object.weak_unref (on_last_view_destroyed);
            }
        }
      
      if (new_view is Bamf.Window)
        {
          unowned Bamf.Window win = new_view as Bamf.Window;
          bool allows_resize = false;
          bool is_maximized  = false;

          global_shell.get_window_details (win.get_xid (),
                                           out allows_resize,
                                           out is_maximized);

          if (is_maximized)
            {
              appname.hide ();
              close.show ();
              minimize.show ();
              unmaximize.show ();
            }
          else
            {
              appname.show ();
              close.hide ();
              minimize.hide ();
              unmaximize.hide ();
            }

          last_xid = win.get_xid ();
       
          unowned Bamf.Application? app = matcher.get_active_application ();
          if (app is Bamf.Application)
            {
              /* We lookup sync because we know that the launcher has already
               * done it
               */

              AppInfo? info = appinfo.lookup (app.get_desktop_file ());
              if (info != null)
                {
                  string display_name = info.get_display_name ();
                  display_name = display_name.split (" ")[0];
                  appname.set_markup (FORMAT.printf (display_name));
                }
              else
                appname.set_markup (FORMAT.printf (win.get_name ()));
            }
          else
            {
              appname.set_markup (FORMAT.printf (win.get_name ()));
            }

          _last_view = new_view;
          _last_view.weak_ref (on_last_view_destroyed);
        }
      else
        {
          _last_view = null;
        }
    }

    private void on_last_view_destroyed (Object object)
    {
      if (object == _last_view)
        {
          _last_view = null;
          appname.set_markup ("");
          appname.hide ();
          close.hide ();
          minimize.hide ();
          unmaximize.hide ();
        }
    }

    private override void get_preferred_width (float     for_height,
                                               out float min_width,
                                               out float nat_width)
    {
      min_width = 72.0f;
      nat_width = min_width;
    }
  }

  public class WindowButton : Ctk.Button
  {
    public static const string AMBIANCE = "/usr/share/themes/Ambiance/metacity-1";
    public static const string AMBIANCE_BETA = "/usr/share/themes/Ambiance-maverick-beta/metacity-1";

    public string filename { get; construct; }
    public Clutter.Actor bg;

    private bool using_beta = false;
    private int  icon_size = 18;
    private string directory = AMBIANCE;

    public WindowButton (string filename)
    {
      Object (filename:filename);
    }

    construct
    {
      if (using_beta = FileUtils.test (AMBIANCE_BETA, FileTest.EXISTS))
        {
          icon_size = 19;
          directory = AMBIANCE_BETA;
        }
      try {
        bg = new Ctk.Image.from_filename (icon_size,
                                          directory +
                                          "/" +
                                          filename +
                                          ".png");
        set_background_for_state (Ctk.ActorState.STATE_NORMAL, bg);
        bg.show ();

        bg = new Ctk.Image.from_filename (icon_size,
                                          directory +
                                          "/" +
                                          filename + 
                                          "_focused_prelight.png");
        set_background_for_state (Ctk.ActorState.STATE_PRELIGHT, bg);
        bg.show ();

        bg = new Ctk.Image.from_filename (icon_size,
                                          directory +
                                          "/" +
                                          filename + 
                                          "_focused_pressed.png");
        set_background_for_state (Ctk.ActorState.STATE_ACTIVE, bg);
        bg.show ();

      } catch (Error e) {
        warning (@"Unable to load window button theme: You need Ambiance installed: $(e.message)");
      }
    }

    private override void get_preferred_width (float     for_height,
                                               out float min_width,
                                               out float nat_width)
    {
      min_width = icon_size;
      nat_width = min_width;
    }

    private override void get_preferred_height (float     for_width,
                                                out float min_height,
                                                out float nat_height)
    {
      min_height = icon_size;
      nat_height = min_height;
    }
  }
}
