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

namespace Unity.Homescreen
{
	public class ActivityStore : Ctk.Bin
	{
		private Ctk.VBox      vbox;
		private Ctk.Image     icon;
		private Clutter.Text  primary_label;
		private Clutter.Text  secondary_label;

		public ActivityStore ()
		{
			Clutter.Color color = {255, 255, 255, 255};

			stdout.printf ("ActivityStore() called\n");

			this.vbox = new Ctk.VBox (0);
			this.vbox.width = 128;
			this.icon = new Ctk.Image.from_stock (128, "applications-internet");
			this.primary_label = new Clutter.Text ();
			this.primary_label.width = 128;
			this.primary_label.text = "Web";
			this.primary_label.justify = true;
			this.primary_label.color = color;
			this.primary_label.ellipsize = Pango.EllipsizeMode.NONE;
			this.primary_label.line_wrap = false;
			this.secondary_label = new Clutter.Text ();
			this.secondary_label.width = 128;
			this.secondary_label.text = "Search, Suft & Download";
			this.secondary_label.justify = true;
			this.secondary_label.color = color;
			this.secondary_label.ellipsize = Pango.EllipsizeMode.END;
			this.secondary_label.line_wrap = true;

			this.vbox.add_actor (this.icon);
			this.vbox.add_actor (this.primary_label);
			this.vbox.add_actor (this.secondary_label);

			add_actor (this.vbox);
			show_all ();

			this.vbox.x            = 100;
			this.vbox.y            = 50;
			this.icon.x            = 100;
			this.icon.y            = 50;
			this.primary_label.x   = 100;
			this.primary_label.y   = 128+50;
			this.secondary_label.x = 100;
			this.secondary_label.y = 128+50+20;

			/*"/tmp/music-icon.svg"
			"Music",
			"Jukebox, Radio & Podcasts",

			"/tmp/video-icon.svg"
			"Videos",
			"YouTube & More",

			"/tmp/photos-icon.svg"
			"Photos",
			"Organize, Edit & Share",

			"/tmp/work-icon.svg"
			"Work",
			"Office Documents, Spreadsheets & Presentations",

			"/tmp/email-icon.svg"
			"Email",
			"Read & Write Email",

			"/tmp/chat-icon.svg"
			"Chat",
			"AIM, Yahoo, Skype & MSN",

			"/tmp/get-apps-icon.svg"
			"Get New Apps",
			"Ubuntu Software Center",*/
		}

		construct
		{
			stdout.printf ("ActivityStore() constructor called\n");
		}
	}
}
