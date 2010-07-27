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
      /* Create the correct results view */
      if (renderer is Clutter.Actor)
        {
          renderer.destroy ();
        }

      if (active_entry is PlaceEntry)
        {
          active_entry.active = false;
        }
      active_entry = entry;
      entry.active = true;

      renderer = lookup_renderer (entry.entry_renderer_name);
      content_box.pack (renderer, true, true);
      renderer.set_models (entry.entry_groups_model,
                           entry.entry_results_model,
                           entry.entry_renderer_hints);
      renderer.show ();

      /* Update the search bar */
      search_bar.set_active_entry_view (entry, 0, section_id);
    }

    private Unity.Place.Renderer lookup_renderer (string renderer)
    {
      return new DefaultRenderer ();
    }
  }
}

