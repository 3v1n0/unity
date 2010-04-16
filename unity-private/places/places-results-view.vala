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
using Gee;
using Unity.Places.Application;

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

  public class ResultsView : Ctk.Box, Unity.Places.PlaceView
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

    private GroupView   group_view;
    private Ctk.VBox    results_view;

    private HashMap<string, ApplicationGroup> groups;

    public ResultsView ()
    {
      Object (orientation:Ctk.Orientation.VERTICAL);
    }

    construct
    {
      this.group_view = new GroupView ();
      this.add_actor (group_view);
      this.group_view.show ();

      this.results_view = new Ctk.VBox (12);
      this.results_view.homogeneous = false;
      this.add_actor (results_view);
      this.results_view.show ();

      this.groups = new HashMap<string, ApplicationGroup> ();
    }

    /**
     * CLUTTER RELATED METHODS
     **/
    private override void allocate (Clutter.ActorBox        box,
                                    Clutter.AllocationFlags flags)
    {
      Clutter.ActorBox child_box = {0, 0, 0, 0};
      float width, height;
      float padding = 24;
      float natheight = 0;

      width = box.x2 - box.x1 - padding * 2;
      height = box.y2 - box.y1 - padding * 2;

      child_box.x1 = padding;
      child_box.x2 = width;
      child_box.y1 = padding;
      child_box.y2 = padding * 2;
      this.group_view.allocate (child_box, flags);

      this.results_view.get_preferred_height (width,
                                              out natheight,
                                              out natheight);
      child_box.y1 += child_box.y2;
      child_box.y2 = child_box.y1 + natheight;
      this.results_view.allocate (child_box, flags);
    }

    /**
     * MODEL RELATED METHODS
     **/

    public void init_with_properties (HashTable<string, string> props)
    {
      string group_model_path = props.lookup("groups-model");
      string results_model_path = props.lookup("results-model");

      this.groups_model = new Dbus.Model.with_name (group_model_path);
      this.results_model = new Dbus.Model.with_name (results_model_path);

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
      this.group_view.add (model, iter);
    }

    private void on_group_changed (Dbus.Model model, ModelIter iter)
    {

    }

    private void on_group_removed (Dbus.Model model, ModelIter iter)
    {

    }

    private void on_result_added (Dbus.Model model, ModelIter iter)
    {
      var group_name = model.get_string (iter, ResultsColumns.GROUP);

      ApplicationGroup? group = this.groups[group_name];

      if (group is ApplicationGroup)
        {
          if (group.n_items < 6)
            {
              ApplicationIcon app;
              string name = model.get_string (iter, ResultsColumns.NAME);
              string icon_name = model.get_string (iter, ResultsColumns.ICON_NAME);
              string comment = model.get_string (iter, ResultsColumns.COMMENT);

              app = new ApplicationIcon (48,
                                         name,
                                         icon_name,
                                         comment);

              group.add_icon (app);
            }
        }
      else
        {
          group = new ApplicationGroup (group_name);
          this.results_view.add_actor (group);
          group.show ();

          this.groups[group_name] = group;
        }
    }

    private void on_result_changed (Dbus.Model model, ModelIter iter)
    {

    }

    private void on_result_removed (Dbus.Model model, ModelIter iter)
    {

    }
  }

  private class GroupView : Ctk.Box
  {
    public GroupView ()
    {
      Object (orientation:Ctk.Orientation.HORIZONTAL,
              spacing:12,
              homogeneous:false);
    }

    construct
    {

    }

    public void add (Dbus.Model model, ModelIter iter)
    {
      var text = new GroupLabel (model, iter);

      this.add_actor (text);
      text.show ();
    }
  }

  private class GroupLabel : Ctk.Text
  {
    public unowned Dbus.Model model { get; construct; }
    public unowned ModelIter  iter  { get; construct; }

    private Clutter.Actor bg;

    public GroupLabel (Dbus.Model model, ModelIter iter)
    {
      Object (model:model,
              iter:iter,
              text:model.get_string (iter, GroupColumns.NAME));
    }

    construct
    {
      Clutter.Color white = { 0xff, 0xff, 0xff, 0x33 };

      var rect = new Clutter.Rectangle.with_color (white);

      this.bg = (Clutter.Actor)rect;
      this.bg.set_parent (this);
      this.bg.show ();
    }

    private override void allocate (Clutter.ActorBox        box,
                                    Clutter.AllocationFlags flags)
    {
      base.allocate (box, flags);

      Clutter.ActorBox child_box = { 0, 0, 0, 0 };
      child_box.x2 = box.x2 - box.x1;
      child_box.y2 = box.y2 - box.y1 - 3;

      this.bg.allocate (child_box, flags);
    }

    private override void map ()
    {
      base.map ();
      this.bg.map ();
    }

    private override void unmap ()
    {
      base.unmap ();
      this.bg.unmap ();
    }

    private override void show ()
    {
      base.show ();
      this.bg.show ();
    }

    private override void hide ()
    {
      base.hide ();
      this.bg.hide ();
    }

    private override void paint ()
    {
      this.bg.paint ();
      base.paint ();
    }
  }
}

