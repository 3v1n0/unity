/* -*- Mode: vala; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/*
 * Copyright (C) 2009 Canonical Ltd
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
 * Authored by Gordon Allott <gord.allott@canonical.com>
 *
 */

namespace Unity.Launcher
{
  const string HONEYCOMB_MASK_FILE = Unity.PKGDATADIR
    + "/honeycomb-mask.png";
  const string MENU_BG_FILE = Unity.PKGDATADIR
    + "/tight_check_4px.png";

  const uint SHORT_DELAY = 400;
  const uint MEDIUM_DELAY = 800;
  const uint LONG_DELAY = 1600;

  class LauncherChild : ScrollerChild
  {

    private Ctk.Actor processed_icon;
    private ThemeImage focused_indicator;
    private ThemeImage running_indicator;
    private Gdk.Pixbuf honeycomb_mask;

    construct
    {
      load_textures ();
    }

    /* private methods */
    private void load_textures ()
    {
      focused_indicator = new ThemeImage ("application-selected");
      running_indicator = new ThemeImage ("application-running");

      focused_indicator.set_parent (this);
      running_indicator.set_parent (this);
      focused_indicator.set_opacity (0);
      running_indicator.set_opacity (0);

      try
        {
          honeycomb_mask = new Gdk.Pixbuf.from_file(HONEYCOMB_MASK_FILE);
        }
      catch (Error e)
        {
          warning ("Unable to load asset %s: %s",
                   HONEYCOMB_MASK_FILE,
                   e.message);
        }

        processed_icon = new Ctk.Image (48);
        processed_icon.set_size (48, 48);
        processed_icon.set_parent (this);
    }


    /* clutter overrides */
    public override void get_preferred_width (float for_height,
                                                out float minimum_width,
                                                out float natural_width)
    {
      natural_width = 58;
      minimum_width = 58;
      return;
    }

    public override void get_preferred_height (float for_width,
                                               out float minimum_height,
                                               out float natural_height)
    {
      natural_height = 58;
      minimum_height = 58;
    }

    public override void allocate (Clutter.ActorBox box, Clutter.AllocationFlags flags)
    {
      float x, y;
      x = 0;
      y = 0;
      base.allocate (box, flags);

      Clutter.ActorBox child_box = Clutter.ActorBox ();

      //allocate the running indicator first
      float width, height, n_width, n_height;
      running_indicator.get_preferred_width (48, out n_width, out width);
      running_indicator.get_preferred_height (48, out n_height, out height);
      child_box.x1 = 0;
      child_box.y1 = (box.get_height () - height) / 2.0f;
      child_box.x2 = child_box.x1 + width;
      child_box.y2 = child_box.y1 + height;
      running_indicator.allocate (child_box, flags);
      x += child_box.get_width ();

      //allocate the icon
      processed_icon.get_preferred_width (48, out width, out n_width);
      processed_icon.get_preferred_height (48, out height, out n_height);
      child_box.x1 = (box.get_width () - width) / 2.0f;
      child_box.y1 = y;
      child_box.x2 = child_box.x1 + 48;
      child_box.y2 = child_box.y1 + height;
      processed_icon.allocate (child_box, flags);

      //allocate the focused indicator
      focused_indicator.get_preferred_width (48, out n_width, out width);
      focused_indicator.get_preferred_height (48, out n_height, out height);
      child_box.x1 = box.get_width () - width;
      child_box.y1 = (box.get_height () - height) / 2.0f;
      child_box.x2 = child_box.x1 + width;
      child_box.y2 = child_box.y1 + height;
      focused_indicator.allocate (child_box, flags);

    }

    public override void pick (Clutter.Color color)
    {
      base.pick (color);
      processed_icon.paint ();
    }

    public override void paint ()
    {
      focused_indicator.paint ();
      running_indicator.paint ();

      processed_icon.paint ();
    }

    public override void map ()
    {
      base.map ();
      running_indicator.map ();
      focused_indicator.map ();
      processed_icon.map ();
    }

    public override void unmap ()
    {
      base.unmap ();
      running_indicator.map ();
      focused_indicator.map ();
      processed_icon.map ();
    }
  }
}



