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

namespace Unity.Quicklauncher
{
  public class View : Ctk.Bin
  {
    private Shell shell;

    private Manager manager;

    public View (Shell shell)
    {
      this.shell = shell;
    }

    construct
    {
      this.manager = new Manager ();
      this.add_actor (manager);
    }

    public new float get_width ()
    {
      return 58;
    }
  }
}
