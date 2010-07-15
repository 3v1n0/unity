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
    Ctk.LayerActor ql_background;
    int            old_width;
    int            old_height;
    float          cached_x; // needed to fix LP: #525905
    float          cached_y; // needed to fix LP: #526335

    public override void
    paint ()
    {
      base.paint ();
    }

   private override void
    allocate (Clutter.ActorBox        box,
              Clutter.AllocationFlags flags)
    {
      base.allocate (box, flags);
    }

    construct
    {
      this.set_spacing (2);
      this.set_content_padding (4);
      this.set_padding (16);
    }
  }
}
