/* -*- Mode: vala; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
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
 * Authored by Mikkel Kamstrup Erlandsen <mikkel.kamstrup@canonical.com>
 *
 */
using Unity;
using Unity.Place;
using Unity.Testing;

namespace Unity.Tests.Unit
{
  public class PlaceSuite
  {
    public PlaceSuite ()
    {
      Test.add_data_func ("/Unit/Place/Empty",
                          PlaceSuite.test_empty_controller);
      Test.add_data_func ("/Unit/Place/OneEntry",
                          PlaceSuite.test_one_entry);
      Test.add_data_func ("/Unit/Place/TwoEntries",
                          PlaceSuite.test_two_entries);
      Test.add_data_func ("/Unit/Place/LocalModels",
                          PlaceSuite.test_local_models);
    }

    internal static void test_empty_controller()
    {
      var ctl = new Controller("/org/ayatana/unity/testplace");
      assert (ctl is Controller);
      assert (!ctl.exported);
      assert (ctl.dbus_path == "/org/ayatana/unity/testplace");
      assert (ctl.num_entries() == 0);
      assert (ctl.get_entry ("no such entry") == null);
    }

    internal static void test_one_entry()
    {
      var entry_path = "/org/ayatana/unity/testplace/testentry1";
      var entry = new EntryInfo(entry_path);
      assert (entry is EntryInfo);
      assert (entry.dbus_path == entry_path);

      var ctl = new Controller("/org/ayatana/unity/testplace");
      assert (ctl.num_entries() == 0);

      /* Assert that we can add one entry */
      ctl.add_entry (entry);
      assert (ctl.num_entries() == 1);
      assert (ctl.get_entry (entry_path) == entry);

      var entry_paths = ctl.get_entry_paths();
      assert (entry_paths.length == 1);
      assert (entry_paths[0] == entry_path);

      var entries = ctl.get_entries ();
      assert (entries.length == 1);
      assert (entries[0] == entry);

      /* Assert that we can remove it again */
      ctl.remove_entry (entry_path);
      assert (ctl.num_entries() == 0);
      assert (ctl.get_entry (entry_path) == null);
    }

    internal static void test_two_entries()
    {
      var entry_path1 = "/org/ayatana/unity/testplace/testentry1";
      var entry_path2 = "/org/ayatana/unity/testplace/testentry2";
      var entry1 = new EntryInfo(entry_path1);
      var entry2 = new EntryInfo(entry_path2);

      var ctl = new Controller("/org/ayatana/unity/testplace");
      assert (ctl.num_entries() == 0);

      ctl.add_entry (entry1);
      assert (ctl.num_entries() == 1);

      ctl.add_entry (entry2);
      assert (ctl.num_entries() == 2);

      assert (ctl.get_entry (entry_path1) == entry1);
      assert (ctl.get_entry (entry_path2) == entry2);

      var entry_paths = ctl.get_entry_paths();
      assert (entry_paths.length == 2);
      assert (entry_paths[0] == entry_path1);
      assert (entry_paths[1] == entry_path2);

      var entries = ctl.get_entries ();
      assert (entries.length == 2);
      assert (entries[0] == entry1);
      assert (entries[1] == entry2);

      /* Assert we can't add them twice */

      ctl.add_entry (entry1);
      assert (ctl.num_entries() == 2);

      ctl.add_entry (entry2);
      assert (ctl.num_entries() == 2);

      /* Assert we can remove them */

      ctl.remove_entry (entry_path1);
      assert (ctl.num_entries() == 1);
      assert (ctl.get_entry (entry_path1) == null);
      assert (ctl.get_entry (entry_path2) == entry2);

      ctl.remove_entry (entry_path2);
      assert (ctl.num_entries() == 0);
      assert (ctl.get_entry (entry_path1) == null);
      assert (ctl.get_entry (entry_path2) == null);

      entry_paths = ctl.get_entry_paths();
      assert (entry_paths.length == 0);

      entries = ctl.get_entries ();
      assert (entries.length == 0);
    }

    internal static void test_local_models ()
    {
      var entry = new EntryInfo("/foo/bar");
      Dee.Model sections_model = new Dee.SequenceModel(2,
                                                       typeof(string),
                                                       typeof(string));
      entry.sections_model = sections_model;
      assert (entry.sections_model == sections_model);
      assert (sections_model.get_n_rows() == 0);

      var renderer = entry.entry_renderer_info;
      assert (renderer is RendererInfo);
      Dee.Model groups_model = new Dee.SequenceModel(3,
                                                     typeof(string),
                                                     typeof(string),
                                                     typeof(string));
      renderer.groups_model = groups_model;
      assert (renderer.groups_model == groups_model);
      assert (groups_model.get_n_rows() == 0);

      Dee.Model results_model = new Dee.SequenceModel(6,
                                                      typeof(string),
                                                      typeof(string),
                                                      typeof(uint),
                                                      typeof(string),
                                                      typeof(string),
                                                      typeof(string));
      renderer.results_model = results_model;
      assert (renderer.results_model == results_model);
      assert (results_model.get_n_rows() == 0);
    }

  }
}
