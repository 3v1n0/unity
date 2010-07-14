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

    public PlaceHomeEntry (Shell shell)
    {
      Object (shell:shell);
    }

    construct
    {
      entry_renderer_name = "boo";

      _sections_model = new Dee.SequenceModel (2, typeof (string), typeof (string));
    }

    /*
     * Methods
     */
    public new void connect ()
    {
      online = true;
    }

    public void set_search (string search, HashTable<string, string> hints)
    {
      debug ("Global search %s", search);
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

