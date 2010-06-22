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
  public class DefaultRendererGroup : Ctk.Box
  {
    static const float PADDING = 0.0f;
    static const int   SPACING = 0;

    public uint      group_id       { get; construct; }
    public string    group_renderer { get; construct; }
    public string    display_name   { get; construct; }
    public string    icon_hint      { get; construct; }
    public Dee.Model results        { get; construct; }

    private Ctk.HBox      title_box;
    private Ctk.Image     icon;
    private Ctk.Text      text;
    private Ctk.Image     expander;
    private Ctk.IconView  renderer;

    public DefaultRendererGroup (uint      group_id,
                                 string    group_renderer,
                                 string    display_name,
                                 string    icon_hint,
                                 Dee.Model results)
    {
      Object (group_id:group_id,
              group_renderer:group_renderer,
              display_name:display_name,
              icon_hint:icon_hint,
              results:results);
    }

    construct
    {
      padding = { PADDING, PADDING, PADDING , PADDING};
      orientation = Ctk.Orientation.VERTICAL;
      spacing = SPACING;
      homogeneous = false;
      hide ();

      title_box = new Ctk.HBox (12);
      pack (title_box, false, true);
      title_box.show ();

      icon = new Ctk.Image (24);
      //title_box.pack (icon, false, true);
      icon.show ();

      text = new Ctk.Text (display_name);
      title_box.pack (text, true, true);
      text.show ();

      expander = new Ctk.Image (24);
      //title_box.pack (expander, false, true);
      expander.show ();

      var sep = new Clutter.Rectangle.with_color ({ 255, 255, 255, 255 });
      sep.set_height (1);
      pack (sep, false, false);
      sep.show ();

      renderer = new Ctk.IconView ();
      renderer.padding = { 12.0f, 0.0f, 0.0f, 0.0f };
      //renderer.spacing = 30;
      pack (renderer, true, true);
      renderer.show ();

      results.row_added.connect (on_result_added);
      results.row_removed.connect (on_result_removed);
    }

    private override void get_preferred_height (float for_width,
                                           out float min_height,
                                           out float nat_height)
    {
      var children = renderer.get_children ();
      if (children.length () > 0)
        {
          base.get_preferred_height (for_width, out min_height, out nat_height);
          show ();
        }
      else
        {
          min_height = 0;
          nat_height = 0;
          hide ();
        }
    }

    /*
     * Private Methods
     */
    private void on_result_added (Dee.ModelIter iter)
    {
      if (!interesting (iter))
        return;

      var button = new Tile ();
      button.set_label (results.get_string (iter, 4));
      unowned Ctk.Text text = button.get_text ();
      text.ellipsize = Pango.EllipsizeMode.END;
      button.get_image ().set_from_stock ("text-x-preview");
      renderer.add_actor (button);
      button.show ();

      button.set_data<unowned Dee.ModelIter> ("model-iter", iter);
      button.set_data<string> ("uri", "%s".printf (results.get_string (iter, 0)));
      button.clicked.connect ((b) => {
        unowned string uri = b.get_data<unowned string> ("uri");

        print (@"Launching $uri\n");
        try {
          Gtk.show_uri (Gdk.Screen.get_default (),
                        uri,
                        0);
         } catch (GLib.Error e) {
           warning ("Unable to launch: %s", e.message);
         }
      });

      show ();
    }

    private void on_result_removed (Dee.ModelIter iter)
    {
      if (!interesting (iter))
        return;

      var children = renderer.get_children ();
      foreach (Clutter.Actor actor in children)
        {
          unowned Dee.ModelIter i;

          i = actor.get_data<unowned Dee.ModelIter> ("model-iter");
          if (i == iter)
            {
              actor.destroy ();
              break;
            }
        }

      if (children.length () <= 1)
        hide ();
    }

    private bool interesting (Dee.ModelIter iter)
    {
      return (results.get_uint (iter, 2) == group_id);
    }
  }

  public class Tile : Ctk.Button
  {
    public Tile ()
    {
      Object (orientation:Ctk.Orientation.VERTICAL);
    }

    private override void get_preferred_width (float for_height,
                                               out float mwidth,
                                               out float nwidth)
    {
      mwidth = 160.0f;
      nwidth = 160.0f;
    }
  }
}

