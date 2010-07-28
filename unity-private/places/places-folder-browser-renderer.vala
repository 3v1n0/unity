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
  public class FolderBrowserRenderer : Ctk.ScrollView, Unity.Place.Renderer
  {
    static const float PADDING = 12.0f;
    static const int   SPACING = 0;

    private Ctk.VBox box;
    private Dee.Model results_model;

    public FolderBrowserRenderer ()
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
    public void set_models (Dee.Model                   model,
                            Dee.Model                   results,
                            Gee.HashMap<string, string> hints)
    {
      results_model = results;

      var group = new DefaultRendererGroup (0,
                                            "UnityFolderGroupRenderer",
                                            "__you_cant_see_me__",
                                            "gtk-apply",
                                            results);
      box.pack (group, false, true);
    }
  }
}
