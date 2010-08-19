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
  public class DefaultRenderer : Ctk.ScrollView, Unity.Place.Renderer
  {
    static const float PADDING = 12.0f;
    static const int   SPACING = 0;

    private Ctk.VBox box;
    private Dee.Model groups_model;
    private Dee.Model results_model;

    public DefaultRenderer ()
    {
      Object ();
    }

    construct
    {
      padding = { 0.0f, 0.0f, 0.0f, 0.0f };
      box = new Ctk.VBox (SPACING);
      box.padding = { 0.0f, PADDING, 0.0f, PADDING};
      box.homogeneous = false;
      add_actor (box);
      box.show ();
    }

    /*
     * Private Methods
     */
    public void set_models (Dee.Model                   groups,
                            Dee.Model                   results,
                            Gee.HashMap<string, string> hints)
    {
      groups_model = groups;
      results_model = results;

      unowned Dee.ModelIter iter = groups.get_first_iter ();
      while (!groups.is_last (iter))
        {
          on_group_added (groups, iter);

          iter = groups.next (iter);
        }

      groups_model.row_added.connect (on_group_added);
      groups_model.row_removed.connect (on_group_removed);
    }

    private void on_group_added (Dee.Model model, Dee.ModelIter iter)
    {
      string renderer = model.get_string (iter, 0);

      if (renderer == "UnityEmptySearchRenderer")
        {
          var group = new EmptySearchGroup (model.get_position (iter),
                                            results_model);
          box.pack (group, false, true);
          group.activated.connect ((u, m) => { activated (u, m); } );

        }
      else if (renderer == "UnityEmptySectionRenderer")
        {
          var group = new EmptrySectionGroup (model.get_position (iter),
                                              results_model);
          box.pack (group, false, true);
        }
      else
        {
          var group = new DefaultRendererGroup (model.get_position (iter),
                                                model.get_string (iter, 0),
                                                model.get_string (iter, 1),
                                                model.get_string (iter, 2),
                                                results_model);
          group.activated.connect ((u, m) => { activated (u, m); } );
          group.set_data<unowned Dee.ModelIter> ("model-iter", iter);
          box.pack (group, false, true);
        }
    }

    private void on_group_removed (Dee.Model model, Dee.ModelIter iter)
    {
      GLib.List<Clutter.Actor> children = box.get_children ();
      foreach (Clutter.Actor actor in children)
        {
          unowned Dee.ModelIter i = (Dee.ModelIter)actor.get_data<Dee.ModelIter> ("model-iter");
          if (i == iter)
            {
              actor.destroy ();
              break;
            }
        }
    }
  }

  public class EmptySearchGroup : ExpandingBin
  {
    public uint   group_id { get; construct set; }

    public Dee.Model results { get; construct set; }

    public signal void activated (string uri, string mimetype);

    private Ctk.VBox box;

    public EmptySearchGroup (uint group_id, Dee.Model results)
    {
      Object (group_id:group_id, results:results);
    }

    construct
    {
      bin_state = ExpandingBinState.CLOSED;
      unexpanded_height = 0.0f;

      var hbox = new Ctk.HBox (0);
      add_actor (hbox);
      hbox.show ();

      box = new Ctk.VBox (12);
      hbox.pack (box, false, false);
      box.show ();

      results.row_added.connect (on_result_added);
      results.row_removed.connect (on_result_removed);

      opacity = 0;
    }

    private void on_result_added (Dee.ModelIter iter)
    {
      if (!interesting (iter))
        return;
      
      bin_state = ExpandingBinState.EXPANDED;

      string mes = results.get_string (iter, 4);

      var button = new Ctk.Button (Ctk.Orientation.HORIZONTAL);
      button.padding = { 12.0f, 12.0f, 12.0f, 12.0f };
      var text = new Ctk.Text ("");
      text.set_markup ("<big>" + mes + "</big>");
      button.add_actor (text);

      box.pack (button, false, false);

      if (box.get_children ().length () >= 2)
        {
          var bg = new StripeTexture (null);
          button.set_background (bg);

          string uri = results.get_string (iter, 0);
          string mimetype = results.get_string (iter, 3);

          button.clicked.connect (() => { activated (uri, mimetype); });
        }
    }

    private void on_result_removed (Dee.ModelIter iter)
    {
      if (!interesting (iter))
        return;

      var children = box.get_children ();
      foreach (Clutter.Actor child in children)
        {
          box.remove_actor (child);
        }

      bin_state = ExpandingBinState.CLOSED;
    }

    private bool interesting (Dee.ModelIter iter)
    {
      return (results.get_uint (iter, 2) == group_id);
    }
  }


  public class EmptrySectionGroup : ExpandingBin
  {
    public uint   group_id { get; construct set; }

    private Ctk.Text text;
    public Dee.Model results { get; construct set; }

    public signal void activated (string uri, string mimetype);

    public EmptrySectionGroup (uint group_id, Dee.Model results)
    {
      Object (group_id:group_id, results:results);
    }

    construct
    {
      bin_state = ExpandingBinState.CLOSED;
      unexpanded_height = 0.0f;

      padding = { 100.0f, 0.0f, 0.0f, 0.0f };

      text = new Ctk.Text ("");
      text.set_alignment (Pango.Alignment.CENTER);

      add_actor (text);
      text.show ();

      results.row_added.connect (on_result_added);
      results.row_removed.connect (on_result_removed);

      opacity = 0;
    }

    private void on_result_added (Dee.ModelIter iter)
    {
      if (!interesting (iter))
        return;
      
      bin_state = ExpandingBinState.EXPANDED;

      string mes = results.get_string (iter, 4);
      text.set_markup ("<big>" + mes + "</big>");
    }

    private void on_result_removed (Dee.ModelIter iter)
    {
      if (!interesting (iter))
        return;

      bin_state = ExpandingBinState.CLOSED;
    }

    private bool interesting (Dee.ModelIter iter)
    {
      return (results.get_uint (iter, 2) == group_id);
    }
  }
}

