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

namespace Unity.Models
{
	public class Places : Object
	{
		public string name;
		public string icon_name;
		public string tooltip;

		public Places (string name,
			       string icon_name,
			       string tooltip)
		{
			this.name      = name;
			this.icon_name = icon_name;
			this.tooltip   = tooltip;
		}

		construct
		{
		}
	}
}
