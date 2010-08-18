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
    static const float PADDING = 12.0f;
    
    public HomeRenderer ()
    {
      Object ();
    }

    construct
    {
      padding = { 0.0f, 0.0f, 0.0f, 0.0f };
      var icon_view = new Ctk.IconView ();
      icon_view.padding = { 0.0f, PADDING, 0.0f, PADDING};
      add_actor (icon_view);
      icon_view.show ();

      /* Load up the world */

    }

    /*
     * Private Methods
     */
    public void set_models (Dee.Model                   groups,
                            Dee.Model                   results,
                            Gee.HashMap<string, string> hints)
    {

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
      get_image ().stock_id = icon;
      get_image ().size = ICON_SIZE;
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
