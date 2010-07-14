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

    private PlaceBar  place_bar;
    private Ctk.VBox  content_box;

    private PlaceHomeEntry       home_entry;
    private PlaceSearchBar       search_bar;
    private Unity.Place.Renderer renderer;

    private bool is_showing = false;

    public View (Shell shell)
    {
      Object (shell:shell, orientation:Ctk.Orientation.VERTICAL);
    }

    construct
    {
    }

    public void about_to_show ()
    {
      if (_model is PlaceFileModel)
        {
          return;
        }

      _model = new PlaceFileModel () as PlaceModel;

      home_entry = new PlaceHomeEntry (shell);

      place_bar = new PlaceBar (shell, _model);
      pack (place_bar, false, true);
      place_bar.show ();

      place_bar.entry_view_activated.connect (on_entry_view_activated);

      content_box = new Ctk.VBox (4);
      content_box.padding = {
        0.0f,
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

      place_bar.reset ();
      search_bar.reset ();

      on_entry_view_activated (home_entry, 0);
    }

    public void hidden ()
    {
      is_showing = false;
    }

    private void on_entry_view_activated (PlaceEntry entry, int x)
    {
      /* Create the correct results view */
      if (renderer is Clutter.Actor)
        {
          renderer.destroy ();
        }
      renderer = lookup_renderer (entry.entry_renderer_name);
      content_box.pack (renderer, true, true);
      renderer.set_models (entry.entry_groups_model,
                           entry.entry_results_model,
                           entry.entry_renderer_hints);
      renderer.show ();

      /* Update the search bar */
      search_bar.set_active_entry_view (entry,
                                        x == 0 ? 0 : x
                                          - shell.get_launcher_width_foobar ()
                                          - 8);
    }

    private Unity.Place.Renderer lookup_renderer (string renderer)
    {
      return new DefaultRenderer ();
    }
  }
}

