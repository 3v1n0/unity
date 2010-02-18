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
 * Authored by Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */

namespace Unity.Panel
{
  public class HomeButton : Unity.ThemeImage
  {
    public Shell shell { get; construct; }

    public HomeButton (Shell shell)
    {
      Object (icon_name:"distributor-logo",
              reactive:true,
              shell:shell);
    }

    construct
    {
      this.button_release_event.connect (this.on_button_release);
    }

    private bool on_button_release (Clutter.Event event)
    {
      shell.show_unity ();

      return false;
    }
  }
}
