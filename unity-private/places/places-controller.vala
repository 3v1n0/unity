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
 * Authored by Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */

using Gee;

namespace Unity.Places
{
  public class Controller : Object
  {
    /**
     * This class takes care of reading in the places, creating the view and
     * keeping it up-to-date
     **/
    public  Shell shell { get; construct; }
    private View view;

    public Controller (Shell shell)
    {
      Object (shell:shell);
    }

    construct
    {
      view = new View (shell);
    }

    public View get_view ()
    {
      return view;
    }
  }
}

