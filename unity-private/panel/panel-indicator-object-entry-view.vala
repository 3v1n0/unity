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
 * Authored by canonical.com
 *
 */

namespace Unity.Panel.Indicators
{
  public class IndicatorObjectEntryView : Ctk.Box
  {
    public unowned Indicator.ObjectEntry entry { get; construct; }
    public signal void menu_moved (Gtk.MenuDirectionType type);

    public IndicatorObjectEntryView (Indicator.ObjectEntry _entry)
    {
      Object (entry:_entry);
    }

    construct
    {
      // Figure out if you need a label, text or both, create the ctk representations
      // Hook up the appropriate signals
      // Emit menu_moved if the user presses left or right arrow in our menu, we want to chain it up to our parent so it shows the next menu (or either left or right).
    }

    public void show_menu ()
    {
      // Register the menu with PanelMenuManager
      // Make sure all the right signals are connected
    }
  }
}