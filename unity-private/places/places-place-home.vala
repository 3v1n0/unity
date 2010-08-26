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
using Unity;

namespace Unity.Places
{
  public class PlaceHomeEntry : Object, PlaceEntry
  {
    /* Properties */
    public Shell shell { get; construct; }

    public string name        { get; construct set; }
    public string icon        { get; construct set; }
    public string description { get; construct set; }

    public uint     position  { get; set; }
    public string[] mimetypes { get; set; }
    public bool     sensitive { get; set; }


    public Gee.HashMap<string, string> hints { get; set; }

    public unowned Place? parent { get; construct set; }
    public bool online { get; construct set; }
    public bool active { get; set; }

    private Dee.Model _sections_model;
    public Dee.Model? sections_model {
      get { return _sections_model; }
      set { _sections_model = value; }
    }

    /* Entry rendering info */
    public string     entry_renderer_name { get; set; }
    public Dee.Model? entry_groups_model  { get; set; }
    public Dee.Model? entry_results_model { get; set; }
    public Gee.HashMap<string, string>? entry_renderer_hints { get; set; }

    /* Global rendering info */
    public string global_renderer_name { get; set; }

    public Dee.Model?  global_groups_model { set; get; }
    public Dee.Model?  global_results_model { set; get; }
    public Gee.HashMap<string, string>? global_renderer_hints { get; set; }

    public PlaceModel place_model { get; set construct; }

    private Gee.HashMap<PlaceEntry?, uint> entry_group_map;

    public PlaceHomeEntry (Shell shell, PlaceModel model)
    {
      Object (shell:shell, place_model:model);
    }

    construct
    {
      entry_group_map = new Gee.HashMap<PlaceEntry?, uint> ();

      _sections_model = new Dee.SequenceModel (2, typeof (string), typeof (string));
      entry_renderer_name = "UnityHomeScreen";

      entry_groups_model = new Dee.SequenceModel (3,
                                                  typeof (string),
                                                  typeof (string),
                                                  typeof (string));

      place_model.place_added.connect (on_place_added);


      entry_results_model = new Dee.SequenceModel (6,
                                                   typeof (string),
                                                   typeof (string),
                                                   typeof (uint),
                                                   typeof (string),
                                                   typeof (string),
                                                   typeof (string));
      entry_renderer_hints = new Gee.HashMap<string, string> ();

      foreach (Place place in place_model)
        on_place_added (place);
    }

    /*
     * Private
     */
    private void on_place_added (Place place)
    {
      foreach (PlaceEntry entry in place.get_entries ())
        {
          unowned Dee.ModelIter iter;
          iter = entry_groups_model.append (0, "UnityLinkGroupRenderer",
                                            1, entry.name,
                                            2, entry.icon,
                                            -1);

          entry_group_map[entry] = entry_groups_model.get_position (iter);

          entry.updated.connect (() => {
            entry.global_results_model.row_added.connect ((it) => {
              var _model = entry.global_results_model;

              unowned Dee.ModelIter i = entry.entry_groups_model.get_iter_at_row (_model.get_uint (it, 2));

              if (entry.entry_groups_model.get_string (i, 0)
                  == "UnityEmptySearchRenderer")  
                {
                  return;
                }

              entry_results_model.append (0, _model.get_string (it, 0),
                                          1, _model.get_string (it, 1),
                                          2, entry_group_map[entry],
                                          3, _model.get_string (it, 3),
                                          4, _model.get_string (it, 4),
                                          5, _model.get_string (it, 5),
                                          -1);

            });

            entry.global_results_model.row_removed.connect ((it) => {
            var _model = entry.global_results_model;

            string uri = _model.get_string (it, 0);

            unowned Dee.ModelIter i = entry_results_model.get_first_iter ();
            while (i != null && !entry_results_model.is_last (i))
              {
                if (entry_results_model.get_string (i, 0) == uri)
                 {
                   entry_results_model.remove (i);
                   break;
                 }

                i = _model.next (i);
              }
            });
          });
        }
    }

    /*
     * Public Methods
     */
    public new void connect ()
    {
      online = true;
    }

    public void set_search (string search, HashTable<string, string> hints)
    {
      var old_renderer = entry_renderer_name;

      if (search == "" || search == null)
        {
          entry_renderer_name = "UnityHomeScreen";
        }
      else
        {
          entry_renderer_name = "UnityHomeResultsRenderer";

          foreach (Gee.Map.Entry<PlaceEntry, uint> e in entry_group_map.entries)
            {
              PlaceEntry? entry = e.key;

              if (entry != null)
                {
                  entry.set_global_search (search, hints);
                }
            }
        }
      
      if (old_renderer != entry_renderer_name)
        {
          updated ();
          renderer_info_changed ();
        }
    }

    public void set_active_section (uint section_id)
    {

    }

    public void set_global_search (string                    search,
                                   HashTable<string, string> hints)
    {

    }
  }
}

