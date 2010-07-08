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
using Unity;

namespace Unity.Places
{
  public class PlaceHome: Ctk.Box
  {
    /* Properties */
    public Shell shell { get; construct; }

    public PlaceHome (Shell shell)
    {
      Object (shell:shell, orientation:Ctk.Orientation.VERTICAL);
    }

    construct
    {
      var rect = new Clutter.Rectangle.with_color ({ 255, 255, 255, 100 });
      set_background (rect);
    }
  }
}

