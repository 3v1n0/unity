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
  public class PlaceEntryView : Ctk.Image
  {
    static const int WIDTH = 80;
    /* Properties */
    public PlaceEntry entry { get; construct; }

    public PlaceEntryView (PlaceEntry entry)
    {
      Object (entry:entry,
              size:48,
              reactive:true);

      set_from_filename (entry.icon);
    }

    construct
    {
    }

    private override void get_preferred_width (float     for_height,
                                               out float min_width,
                                               out float nat_width)
    {
      min_width = (float) WIDTH;
      nat_width = (float) WIDTH;
    }
  }
}

