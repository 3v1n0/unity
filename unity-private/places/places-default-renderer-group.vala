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
    private ExpandingBin  expanding_bin;
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

      title_box = new Ctk.HBox (8);
      pack (title_box, false, false);
      title_box.show ();
      title_box.reactive = true;

      icon = new Ctk.Image (24);
      title_box.pack (icon, false, false);
      icon.show ();

      text = new Ctk.Text (display_name);
      title_box.pack (text, true, true);
      text.show ();

      expander = new Ctk.Image (24);
      title_box.pack (expander, false, true);
      expander.show ();

      var rect = new Clutter.Rectangle.with_color ({ 255, 255, 255, 255 });
      rect.height = 1;
      pack (rect, false, false);
      rect.show ();

      expanding_bin = new ExpandingBin ();
      pack (expanding_bin, true, true);
      expanding_bin.bin_state = ExpandingBinState.UNEXPANDED;
      expanding_bin.show ();

      title_box.button_release_event.connect (() => {
        expanding_bin.bin_state = (expanding_bin.bin_state == ExpandingBinState.UNEXPANDED ? ExpandingBinState.EXPANDED : ExpandingBinState.UNEXPANDED);
      });

      renderer = new Ctk.IconView ();
      renderer.padding = { 12.0f, 0.0f, 0.0f, 0.0f };
      renderer.spacing = 24;
      expanding_bin.add_actor (renderer);
      renderer.show ();
      renderer.set ("auto-fade-children", true);

      unowned Dee.ModelIter iter = results.get_first_iter ();
      while (!results.is_last (iter))
        {
          if (interesting (iter))
            on_result_added (iter);

          iter = results.next (iter);
        }

      results.row_added.connect (on_result_added);
      results.row_removed.connect (on_result_removed);
    }

    private override void allocate (Clutter.ActorBox        box,
                                    Clutter.AllocationFlags flags)
    {
      base.allocate (box, flags);

      var children = renderer.get_children ();
      var child = children.nth_data (0) as Clutter.Actor;
      if (child is Clutter.Actor &&
          child.height != expanding_bin.unexpanded_height)
        expanding_bin.unexpanded_height = child.height;
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

      var button = new Tile (iter,
                             results.get_string (iter, 0),
                             results.get_string (iter, 1),
                             results.get_string (iter, 3),
                             results.get_string (iter, 4),
                             results.get_string (iter, 5));
      renderer.add_actor (button);
      button.show ();

      show ();
    }

    private void on_result_removed (Dee.ModelIter iter)
    {
      if (!interesting (iter))
        return;

      var children = renderer.get_children ();
      foreach (Clutter.Actor actor in children)
        {
          Tile tile = actor as Tile;

          if (tile.iter == iter)
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
    static const int ICON_SIZE = 48;
    static const string DEFAULT_ICON = "text-x-preview";

    public unowned Dee.ModelIter iter { get; construct; }

    public string  display_name { get; construct; }
    public string? icon_hint    { get; construct; }
    public string  uri          { get; construct; }
    public string? mimetype     { get; construct; }
    public string? comment      { get; construct; }

    public Tile (Dee.ModelIter iter,
                 string        uri,
                 string?       icon_hint,
                 string?       mimetype,
                 string        display_name,
                 string?       comment)
    {
      Object (orientation:Ctk.Orientation.VERTICAL,
              iter:iter,
              display_name:display_name,
              icon_hint:icon_hint,
              uri:uri,
              mimetype:mimetype,
              comment:comment);
    }

    construct
    {
      set_label (display_name);

      unowned Ctk.Text text = get_text ();
      text.ellipsize = Pango.EllipsizeMode.END;

      set_icon ();
    }

    private override void get_preferred_width (float for_height,
                                               out float mwidth,
                                               out float nwidth)
    {
      mwidth = 150.0f;
      nwidth = 150.0f;
    }

    private override void clicked ()
    {
      clicked_handler.begin ();
    }

    private async void clicked_handler ()
    {
      global_shell.hide_unity ();

      if (uri.has_prefix ("application://"))
        {
          var id = uri.offset ("application://".length);

          AppInfo info;
          try {
            var appinfos = AppInfoManager.get_instance ();
            info = yield appinfos.lookup_async (id);
          } catch (Error ee) {
            warning ("Unable to read .desktop file '%s': %s", uri, ee.message);
            return;
          }

          if (info is AppInfo)
            {
              try {
                info.launch (null,null);
              } catch (Error e) {
                warning ("Unable to launch desktop file %s: %s\n",
                         id,
                         e.message);
              }
            }
          else
            {
              warning ("%s is an invalid DesktopAppInfo id\n", id);
            }
          return;
        }

      try {
        Gtk.show_uri (Gdk.Screen.get_default (),
                      uri,
                      0);
       } catch (GLib.Error eee) {
         warning ("Unable to launch: %s\n", eee.message);
       }
    }

    private void set_icon ()
    {
      var cache = PixbufCache.get_default ();

      if (icon_hint != null && icon_hint != "")
        {
          cache.set_image_from_gicon_string (get_image (), icon_hint, ICON_SIZE);
        }
      else if (mimetype != null && mimetype != "")
        {
          var icon = GLib.g_content_type_get_icon (mimetype);
          cache.set_image_from_gicon_string (get_image (), icon.to_string(), ICON_SIZE);
        }
      else
        {
          cache.set_image_from_icon_name (get_image (), DEFAULT_ICON, ICON_SIZE);
        }
    }
  }
}

