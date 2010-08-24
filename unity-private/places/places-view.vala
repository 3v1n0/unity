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
 * Authored by Mirco "MacSlow" Müller <mirco.mueller@canonical.com>
 *             Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */
using Unity;

namespace Unity.Places
{
  public class View : Ctk.Box
  {
    public static const float PADDING = 8.0f;
    /* Properties */
    public Shell shell { get; construct; }

    public PlaceModel model { get; construct; }

    private Ctk.VBox  content_box;

    public  PlaceHomeEntry       home_entry;
    public  PlaceSearchBar       search_bar;
    private Unity.Place.Renderer renderer;
    private unowned PlaceEntry?  active_entry = null;

    private bool is_showing = false;

    public View (Shell shell, PlaceModel model)
    {
      Object (shell:shell, orientation:Ctk.Orientation.VERTICAL, model:model);
    }

    construct
    {
      about_to_show ();
    }

    public void about_to_show ()
    {
      if (home_entry is PlaceHomeEntry)
        {
          return;
        }

      home_entry = new PlaceHomeEntry (shell, model);

      content_box = new Ctk.VBox (4);
      content_box.padding = {
        PADDING * 2.5f,
        PADDING,
        0.0f,
        PADDING
      };
      pack (content_box, true, true);
      content_box.show ();

      search_bar = new PlaceSearchBar ();
      content_box.pack (search_bar, false, true);
      search_bar.show ();
    }

    public void shown ()
    {
      is_showing = true;

      search_bar.reset ();

      on_entry_view_activated (home_entry, 0);
    }

    public void hidden ()
    {
      is_showing = false;

      if (active_entry is PlaceEntry)
        {
          active_entry.active = false;
        }
      active_entry = null;
    }

    public void on_entry_view_activated (PlaceEntry entry, uint section_id)
    {
      if (active_entry is PlaceEntry)
        {
          active_entry.active = false;

          if (active_entry != home_entry)
            active_entry.renderer_info_changed.disconnect (on_entry_renderer_info_changed);
        }
      active_entry = entry;
      entry.active = true;

      update_views (entry, section_id);

      entry.renderer_info_changed.connect (on_entry_renderer_info_changed);
    }

    private void update_views (PlaceEntry entry, uint section_id=0)
    {
      /* Create the correct results view */
      if (renderer is Clutter.Actor)
        {
          renderer.destroy ();
        }

      renderer.destroy ();

      renderer = lookup_renderer (entry);
      content_box.pack (renderer, true, true);
      renderer.set_models (entry.entry_groups_model,
                           entry.entry_results_model,
                           entry.entry_renderer_hints);
      renderer.show ();
      renderer.activated.connect (on_result_activated);

      search_bar.set_active_entry_view (entry, 0, section_id);
    }

    private void on_entry_renderer_info_changed (PlaceEntry entry)
    {
      update_views (entry);
    }

    private Unity.Place.Renderer lookup_renderer (PlaceEntry entry)
    {
      string?      browser_path = null;

      /* FIXME: This is meant to be all automated, it's just we havent got
       * there just yet
       */
      if (entry.hints != null)
        {
          foreach (Gee.Map.Entry<string,string> e in entry.hints)
            {
              if (e.key == "UnityPlaceBrowserPath")
                browser_path = e.value;
            }
        }

      if (browser_path != null)
        return new FolderBrowserRenderer ();
      else if (entry.entry_renderer_name == "UnityHomeScreen")
        return new HomeRenderer ();
      else
        return new DefaultRenderer ();
    }

    private void on_result_activated (string uri, string mimetype)
    {
      bool check_with_service = false;

      if (active_entry.parent is Place)
        {
          if (active_entry.parent.uri_regex != null &&
              active_entry.parent.uri_regex.match (uri))
            check_with_service = true;

          if (active_entry.parent.mime_regex != null &&
              active_entry.parent.mime_regex.match (mimetype))
            check_with_service = true;
        }

      if (check_with_service)
        {
          if (active_entry.parent.activate (uri))
            return;        
        }
      activate_normal.begin (uri);
    }

    private async void activate_normal (string uri)
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
  }
}

