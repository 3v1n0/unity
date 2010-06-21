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
  public class DefaultRenderer : Ctk.Box, Unity.Place.Renderer
  {
    static const float PADDING = 12.0f;
    static const int   SPACING = 12;

    private Dee.Model groups_model;
    private Dee.Model results_model;

    public DefaultRenderer ()
    {
      Object ();
    }

    construct
    {
      padding = { PADDING, PADDING, PADDING , PADDING};
      orientation = Ctk.Orientation.VERTICAL;
      spacing = SPACING;
      homogeneous = false;
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

      groups_model.row_added.connect (on_group_added);
      groups_model.row_removed.connect (on_group_removed);
    }

    private void on_group_added (Dee.Model model, Dee.ModelIter iter)
    {
      var group = new DefaultRendererGroup (model.get_position (iter),
                                            model.get_string (iter, 0),
                                            model.get_string (iter, 1),
                                            model.get_string (iter, 2),
                                            results_model);
      group.set_data<unowned Dee.ModelIter> ("model-iter", iter);
      pack (group, false, true);
      group.show ();
    }

    private void on_group_removed (Dee.Model model, Dee.ModelIter iter)
    {
      GLib.List<Clutter.Actor> children = get_children ();
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
}

