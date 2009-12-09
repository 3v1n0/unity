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

namespace Unity.Views
{
	public class Places : Ctk.HBox
	{
		private Gee.ArrayList<Unity.Models.Places> places;

		public Places ()
		{
		}

		construct
		{
			Unity.Models.Places place;

			this.places = new Gee.ArrayList<Unity.Models.Places> ();

			place.name      = "Home";
			place.icon_name = "folder-home";
			place.tooltip   = "Default View";
			this.places.add (place);

			place.name      = "Applications";
			place.icon_name = "applications-games";
			place.tooltip   = "Programs installed locally";
			this.places.add (place);

			place.name      = "Files";
			place.icon_name = "files";
			place.tooltip   = "Your files stored locally";
			this.places.add (place);
		}
	}
}
