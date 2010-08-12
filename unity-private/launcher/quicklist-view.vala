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
 * Authored by Gordon Allott <gord.allott@canonical.com>,
 *             Mirco "MacSlow" Müller <mirco.mueller@canonical.com>
 *
 */

namespace Unity.Launcher
{
  const float LINE_WIDTH             = 0.083f;
  const float LINE_WIDTH_ABS         = 1.5f;
  const float TEXT_HEIGHT            = 1.0f;
  const float MAX_TEXT_WIDTH         = 15.0f;
  const float GAP                    = 0.25f;
  const float MARGIN                 = 0.5f;
  const float BORDER                 = 0.25f;
  const float CORNER_RADIUS          = 0.3f;
  const float CORNER_RADIUS_ABS      = 5.0f;
  const float SHADOW_SIZE            = 1.25f;
  const float ITEM_HEIGHT            = 2.0f;
  const float ITEM_CORNER_RADIUS     = 0.3f;
  const float ITEM_CORNER_RADIUS_ABS = 4.0f;
  const float ANCHOR_HEIGHT          = 1.5f;
  const float ANCHOR_HEIGHT_ABS      = 18.0f;
  const float ANCHOR_WIDTH           = 0.75f;
  const float ANCHOR_WIDTH_ABS       = 10.0f;

  // we call this instead of Ctk.Menu so you can alter this to look right
  public class QuicklistMenu : Ctk.MenuExpandable
  {
    int            last_width;
    int            last_height;
    float          cached_x; // needed to fix LP: #525905
    float          cached_y; // needed to fix LP: #526335

    public override void
    paint ()
    {
      base.paint ();
      // needed to fix LP: #525905
//       float x;
//       float y;
//       this.get_position (out x, out y);
//       if (this.cached_x == 0.0f)
//         this.cached_x = x;
//       if (this.cached_x != x)
//         this.set_position (this.cached_x, y);
      //this.set_anchor_position((int)x, (int)y, 25);
    }

   private override void
    allocate (Clutter.ActorBox        box,
              Clutter.AllocationFlags flags)
    {
      int new_width  = 0;
      int new_height = 0;

      new_width  = (int) (box.x2 - box.x1);
      new_height = (int) (box.y2 - box.y1);

      // exit early if the allocation-width/height didn't change, this is needed
      // because clutter triggers calling allocate even if nothing changed
      if ((this.last_width == new_width) && (this.last_height == new_height))
        return;

      // store the new width/height
      this.last_width  = new_width;
      this.last_height = new_height;

      //debug ("Num Items in Menu %d \n", get_num_items ());

      base.allocate (box, flags);

//       float x;
//       float y;
//       this.get_position(out x, out y);
//       this.compute_style_textures ();
//       this.set_expansion_size_factor (0.0f);
//       this.set_anchor_position ((int)60 + 60, (int)100+48, 25);
//
//       if(get_num_items () > 1)
//         this.animate (Clutter.AnimationMode.LINEAR,
//           100,
//           "expansion-size-factor", 1.0f);


    }

    construct
    {
      this.set_spacing (2);
      this.set_content_padding (0);
      this.set_content_padding_left_right (4);
      this.set_padding (16);

//       Ctk.Padding padding = Ctk.Padding () {
//         left   = (int) (Ctk.em_to_pixel (BORDER + SHADOW_SIZE) + ANCHOR_WIDTH_ABS),
//         right  = (int) Ctk.em_to_pixel (BORDER + SHADOW_SIZE) - 1,
//         top    = (int) Ctk.em_to_pixel (BORDER + SHADOW_SIZE),
//         bottom = (int) Ctk.em_to_pixel (SHADOW_SIZE) + 1
//       };
//       this.set_padding (padding);
//       //this.spacing = (int) Ctk.em_to_pixel (GAP);

      last_width  = -1;
      last_height = -1;
      cached_x   = 0.0f; // needed to fix LP: #525905
      cached_y   = 0.0f; // needed to fix LP: #526335

    }
  }
}
