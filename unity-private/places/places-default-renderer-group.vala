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
  public class DefaultRendererGroup : Ctk.Box
  {
    static const float PADDING = 12.0f;
    static const int   SPACING = 12;

    public string group_renderer { get; construct; }
    public string display_name   { get; construct; }
    public string icon_hint      { get; construct; }

    public DefaultRendererGroup (string group_renderer,
                                 string display_name,
                                 string icon_hint)
    {
      Object (group_renderer:group_renderer,
              display_name:display_name,
              icon_hint:icon_hint);

      print (@"$group_renderer, $display_name, $icon_hint\n");
    }

    construct
    {
      padding = { PADDING, PADDING, PADDING , PADDING};
      orientation = Ctk.Orientation.VERTICAL;
      spacing = SPACING;

      var rect = new Clutter.Rectangle.with_color({0, 255, 0, 255});
      set_background (rect);
      rect.show ();
    }

    /*
     * Private Methods
     */
  }
}

