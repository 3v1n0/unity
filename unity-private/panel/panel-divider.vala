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
 * Authored by Mirco Müller <mirco.mueller@canonical.com>
 *
 */
 
namespace Unity.Panel
{
  public class Divider : Ctk.Image
  {
    private Clutter.Texture bg;

    public Divider ()
    {
      Object (reactive:false);

      Unity.Testing.ObjectRegistry.get_default ().register ("PanelDivider",
                                                            this);
    }

    construct
    {
      try
        {
          bg = new Clutter.Texture.from_file (
                                  "/usr/share/unity/themes/divider.png");
        }
      catch (Error e)
        {
          warning ("Could not load bg-texture for divider: %s", e.message);
        }

      set_background (bg);
    }

    /* the current divider-artwork is two pixels wide */
    private override void get_preferred_width (float     for_height,
                                               out float min_width,
                                               out float nat_width)
    {
      // the width of six pixels is made up of two pixels for the divider itself
      // and four pixels for the padding; the padding needs to be in the image
      // of the artwork, because otherwise clutk/clutter would stretch the two
      // pixel divider across the whole width 
      min_width = 6.0f;
      nat_width = 6.0f;
    }
  }
}
