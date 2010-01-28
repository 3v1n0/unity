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

using Dbus;

namespace Unity.Places.Views
{
  enum GroupColumns
  {
    NAME = 0,
    ACTIVE
  }

  enum ResultsColumns
  {
    NAME = 0,
    COMMENT,
    ICON_NAME,
    GROUP,
    URI
  }

  public class ResultsView : Ctk.Bin, Unity.Places.PlaceView
  {
    /**
     * This is a results view as defined by the design team. It consists of a
     * "group bar" at the top and a vbox at the bottom that contains results
     * of a place or search, grouped by the category that result belongs too.
     *
     * The ResultsView is "powered by" a couple of DbusModel instances, and
     * expects them to be referenced in the properties that are set by the
     * PlaceProxy.
     *
     * A Place service would normally emit the view_changed signal on itself
     * with something like:
     * {
     *   var props = new HashTable<string, string> (str_hash, str_equal);
     *   props.insert ("groups-model", "org.place.MyPlace.GroupModel");
     *   props.insert ("results-model", "org.place.MyPlace.ResultsModel");
     *
     *   this.view_changed ("ResultsView", props);
     * }
     *
     * This would be picked up by the PlaceProxy and this class is initialized
     * with the properties.
     **/

    private Dbus.Model? groups_model;
    private Dbus.Model? results_model;

    public ResultsView ()
    {
      Object ();
    }

    construct
    {

    }

    public void init_with_properties (HashTable<string, string> props)
    {
      string group_model_path = props.lookup("groups-model");
      string results_model_path = props.lookup("results-model");

      this.groups_model = new Dbus.Model.from_path (group_model_path);
      this.results_model = new Dbus.Model.from_path (results_model_path);

      this.groups_model.row_added.connect (this.on_group_added);
      this.groups_model.row_changed.connect (this.on_group_changed);
      this.groups_model.row_removed.connect (this.on_group_removed);

      this.results_model.row_added.connect (this.on_result_added);
      this.results_model.row_changed.connect (this.on_result_changed);
      this.results_model.row_removed.connect (this.on_result_removed);

      this.groups_model.connect ();
      this.results_model.connect ();
    }

    private void on_group_added (Dbus.Model model, ModelIter iter)
    {
      print ("Group: %s\n", model.get_string (iter, GroupColumns.NAME));
    }

    private void on_group_changed (Dbus.Model model, ModelIter iter)
    {

    }

    private void on_group_removed (Dbus.Model model, ModelIter iter)
    {

    }

    private void on_result_added (Dbus.Model model, ModelIter iter)
    {

    }

    private void on_result_changed (Dbus.Model model, ModelIter iter)
    {

    }

    private void on_result_removed (Dbus.Model model, ModelIter iter)
    {

    }
  }
}

