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
    private PlaceModel _model = null;

    /* Properties */
    public Shell shell { get; construct; }

    public PlaceModel model {
      get { return _model; }
      set { _model = value; }
    }

    private Ctk.VBox  content_box;

    private PlaceHomeEntry       home_entry;
    private PlaceSearchBar       search_bar;
    private Unity.Place.Renderer renderer;
    private unowned PlaceEntry?  active_entry = null;

    private bool is_showing = false;

    public View (Shell shell)
    {
      Object (shell:shell, orientation:Ctk.Orientation.VERTICAL);
    }

    construct
    {
      about_to_show ();
    }

    public void about_to_show ()
    {
      if (_model is PlaceFileModel)
        {
          return;
        }

      _model = new PlaceFileModel () as PlaceModel;

      home_entry = new PlaceHomeEntry (shell, _model);

      content_box = new Ctk.VBox (4);
      content_box.padding = {
        26.0f,
        8.0f,
        0.0f,
        shell.get_launcher_width_foobar () + 8.0f
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

    public void on_entry_view_activated (PlaceEntry entry, int x)
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
      search_bar.set_active_entry_view (entry, 0);
    }

    private Unity.Place.Renderer lookup_renderer (string renderer)
    {
      return new DefaultRenderer ();
    }
  }
}

