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
 * Authored by Mirco "MacSlow" Müller <mirco.mueller@canonical.com>
 *
 */

using GLib;

namespace Unity.Places.Default
{
  public class Model : Object
  {
    public string icon_name;
    public string primary_text;
    public string secondary_text;

    public Model (string icon_name,
                  string primary_text,
                  string secondary_text)
    {
      this.icon_name      = icon_name;
      this.primary_text   = primary_text;
      this.secondary_text = secondary_text;
    }

    construct
    {
    }
  }
}
