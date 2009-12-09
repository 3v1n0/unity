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
	public class Default : Ctk.IconView
	{
		private Gee.ArrayList<Unity.Models.Default> activities;

		public Default ()
		{
		}

		construct
		{
			Unity.Models.Default activity;

			this.activities = new Gee.ArrayList<Unity.Models.Default> ();

			activity.icon_name      = "applications-internet";
			activity.primary_text   = "<b>Web</b>";
			activity.secondary_text = "Search, Suft & Download";
			this.activities.add (activity);

			activity.icon_name      = "rhythmbox";
			activity.primary_text   = "<b>Music</b>";
			activity.secondary_text = "Jukebox, Radio & Podcasts";
			this.activities.add (activity);

			activity.icon_name      = "applications-multimedia";
			activity.primary_text   = "<b>Videos</b>";
			activity.secondary_text = "YouTube & More";
			this.activities.add (activity);

			activity.icon_name      = "applications-graphics";
			activity.primary_text   = "<b>Photos</b>";
			activity.secondary_text = "Organize, Edit & Share";
			this.activities.add (activity);

			activity.icon_name      = "applications-office";
			activity.primary_text   = "<b>Work</b>";
			activity.secondary_text = "Office Documents, Spreadsheets & Presentations";
			this.activities.add (activity);

			activity.icon_name      = "evolution";
			activity.primary_text   = "<b>Email</b>";
			activity.secondary_text = "Read & Write Email";
			this.activities.add (activity);

			activity.icon_name      = "empathy";
			activity.primary_text   = "<b>Chat</b>";
			activity.secondary_text = "AIM, Yahoo, Skype & MSN";
			this.activities.add (activity);

			activity.icon_name      = "softwarecenter";
			activity.primary_text   = "<b>Get New Apps</b>";
			activity.secondary_text = "Ubuntu Software Center";
			this.activities.add (activity);
		}
	}
}
