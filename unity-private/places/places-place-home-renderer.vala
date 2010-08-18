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

namespace Unity.Places
{
  public class HomeRenderer : Ctk.Bin, Unity.Place.Renderer
  {
    static const int   SPACING = 60;

    private Gtk.IconTheme? theme = null;
    private Ctk.IconView icon_view;
    
    public HomeRenderer ()
    {
      Object ();
    }

    construct
    {
      padding = { 0.0f, 0.0f, 0.0f, 0.0f };
      icon_view = new Ctk.IconView ();
      icon_view.spacing = SPACING;
      add_actor (icon_view);
      icon_view.show ();

      /* Load up the world */
      var icon = new HomeButton (_("Web"), filename_for_icon ("web"), "");
      icon_view.add_actor (icon);
      icon.show ();
      icon.clicked.connect (() => {
        var client = GConf.Client.get_default ();
        try {
          var exec = client.get_string ("/desktop/gnome/applications/browser/exec");
          if (exec != null)
            Process.spawn_command_line_async (exec);
        } catch (Error e) {
          warning (@"Unable to start web browser: $(e.message)");
        }
        
        global_shell.hide_unity ();

      });

      icon = new HomeButton (_("Music"), filename_for_icon ("music"), "");
      icon_view.add_actor (icon);
      icon.show ();

      icon = new HomeButton (_("Photos & Videos"), filename_for_icon ("photos"), "");
      icon_view.add_actor (icon);
      icon.show ();

      icon = new HomeButton (_("Games"), filename_for_icon ("games"), "");
      icon_view.add_actor (icon);
      icon.show ();

      icon = new HomeButton (_("Email & Chat"), filename_for_icon ("email_and_chat"), "");
      icon_view.add_actor (icon);
      icon.show ();

      icon = new HomeButton (_("Office"), filename_for_icon ("work"), "");
      icon_view.add_actor (icon);
      icon.show ();

      icon = new HomeButton (_("Files & Folders"), filename_for_icon ("work"), "");
      icon_view.add_actor (icon);
      icon.show ();

      icon = new HomeButton (_("Get New Apps"), filename_for_icon ("softwarecentre"), "");
      icon_view.add_actor (icon);
      icon.show ();
      icon.clicked.connect (() => {
        try {
          Process.spawn_command_line_async ("software-center");
        } catch (Error e) {
          warning (@"Unable to start software centre: $(e.message)");
        }
        
        global_shell.hide_unity ();

      });

    }

    public void set_models (Dee.Model                   groups,
                            Dee.Model                   results,
                            Gee.HashMap<string, string> hints)
    {

    }

    /*
     * Private Methods
     */
    private override void allocate (Clutter.ActorBox        box,
                                    Clutter.AllocationFlags flags)
    {
      base.allocate (box, flags);
      float icon_width;
      float icon_height;

      var children = icon_view.get_children ();
      var child = children.nth_data (0) as Clutter.Actor;
      child.get_preferred_size (out icon_width, null,
                                out icon_height, null);

      float left = (box.x2 - box.x1 - (3 * SPACING) - (4 * icon_width))/2.0f;
      float top = (box.y2 - box.y1 - (1 * SPACING) - (2 * icon_height))/2.0f;
      top -= SPACING /2.0f;
            
      Clutter.ActorBox child_box = Clutter.ActorBox  ();
      child_box.x1 = left;
      child_box.x2 = box.x2 - box.x1;
      child_box.y1 = top;
      child_box.y2 = box.y2 - box.y1;

      debug ("%f %f %f %f", child_box.x1, child_box.x2, child_box.y1, child_box.y2);

      icon_view.allocate (child_box, flags);
    }

    private string filename_for_icon (string icon)
    {
      if (theme == null)
        {
          theme = new Gtk.IconTheme ();
          theme.set_custom_theme ("unity-icon-theme");
        }

      string icon_file = "";
      if (theme.has_icon (icon))
        {
          var info = theme.lookup_icon (icon, 128, 0);
          if (info != null)
            {
              var filename = info.get_filename ();
              if (FileUtils.test(filename, FileTest.IS_REGULAR))
                {
                  icon_file = filename;
                }
            }

        }

      return icon_file;
    }
  }

  public class HomeButton : Ctk.Button
  {
    static const int ICON_SIZE = 128;

    public new string name { get; construct; }
    public     string icon { get; construct; }
    public     string exec { get; construct; }
    
    public HomeButton (string name, string icon, string exec)
    {
      Object (name:name,
              icon:icon,
              exec:exec,
              orientation:Ctk.Orientation.VERTICAL);
    }

    construct
    {
      get_image ().size = 128;
      get_image ().filename = icon;
      get_text ().text = name;
    }


    private override void get_preferred_width (float     for_height,
                                               out float mwidth,
                                               out float nwidth)
    {
      mwidth = ICON_SIZE;
      nwidth = ICON_SIZE;
    }
/*
    private override void get_preferred_width (float     for_width,
                                               out float mheight,
                                               out float nheight)
    {

    }*/
  }
}
