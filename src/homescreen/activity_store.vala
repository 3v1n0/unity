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
	public class ActivityWidget : Ctk.VBox
	{
		private Ctk.Image    icon;
		private Clutter.Text primary_label;
		private Clutter.Text secondary_label;

		public ActivityWidget (int    spacing,
				       int    size,
				       string icon_name,
				       string primary_text,
				       string secondary_text)
		{
			Clutter.Color color = {255, 255, 255, 255};
			//Ctk.Padding pad = {padding, padding, padding, padding};
			this.homogeneous = false;

			this.set_reactive (true);
			//this.enter_event.connect (this.on_enter);
			//this.leave_event.connect (this.on_leave);
			//this.clicked_event.connect (this.on_clicked);

			//this.padding = pad;
			this.spacing = spacing;
			//this.width   = size + 32;
			this.icon    = new Ctk.Image.from_stock (size,
								 icon_name);
			this.icon.set_reactive (true);

			this.primary_label = new Clutter.Text ();
			this.primary_label.set_reactive (true);
			this.primary_label.width = size;
			this.primary_label.set_markup (primary_text);
			this.primary_label.justify = true;
			this.primary_label.color = color;
			this.primary_label.ellipsize = Pango.EllipsizeMode.NONE;
			this.primary_label.line_wrap = true;
			this.primary_label.line_alignment = Pango.Alignment.CENTER;

			this.secondary_label = new Clutter.Text ();
			this.secondary_label.set_reactive (true);
			this.secondary_label.width = size + spacing;
			this.secondary_label.text = secondary_text;
			this.secondary_label.justify = false;
			this.secondary_label.color = color;
			//this.secondary_label.ellipsize = Pango.EllipsizeMode.END;
			this.secondary_label.line_wrap = true;
			this.secondary_label.line_alignment = Pango.Alignment.CENTER;
			this.secondary_label.line_wrap_mode = Pango.WrapMode.WORD_CHAR;
			this.enter_event.connect (this.on_enter);
			this.leave_event.connect (this.on_leave);
			this.secondary_label.opacity = 0;

			this.pack (this.icon, false, false);
			this.pack (this.primary_label, false, false);
			this.pack (this.secondary_label, false, false);
		}

		public bool on_enter ()
		{
			this.secondary_label.opacity = 255;
			return false;
		}

		public bool on_leave ()
		{
			this.secondary_label.opacity = 0;
			return false;
		}

		public bool on_clicked ()
		{
			stdout.printf ("on_clicked() called\n");
			return false;
		}
	}

	public class ActivityStore : Ctk.Bin
	{
		public ActivityStore ()
		{
			Ctk.VBox       vbox;
			Ctk.HBox       hbox1;
			Ctk.HBox       hbox2;
			ActivityWidget widget1;
			ActivityWidget widget2;
			ActivityWidget widget3;
			ActivityWidget widget4;
			ActivityWidget widget5;
			ActivityWidget widget6;
			ActivityWidget widget7;
			ActivityWidget widget8;

			widget1 = new ActivityWidget (0,
						      128,
						      "applications-internet",
						      "<b>Web</b>",
						      "Search, Suft & Download");
			widget2 = new ActivityWidget (0,
						      128,
						      "rhythmbox",
						      "<b>Music</b>",
						      "Jukebox, Radio & Podcasts");
			widget3 = new ActivityWidget (0,
						      128,
						      "applications-multimedia",
						      "<b>Videos</b>",
						      "YouTube & More");
			widget4 = new ActivityWidget (0,
						      128,
						      "applications-graphics",
						      "<b>Photos</b>",
						      "Organize, Edit & Share");
			widget5 = new ActivityWidget (0,
						      128,
						      "applications-office",
						      "<b>Work</b>",
						      "Office Documents, Spreadsheets & Presentations");
			widget6 = new ActivityWidget (0,
						      128,
						      "evolution",
						      "<b>Email</b>",
						      "Read & Write Email");
			widget7 = new ActivityWidget (0,
						      128,
						      "empathy",
						      "<b>Chat</b>",
						      "AIM, Yahoo, Skype & MSN");
			widget8 = new ActivityWidget (0,
						      128,
						      "softwarecenter",
						      "<b>Get New Apps</b>",
						      "Ubuntu Software Center");
			vbox = new Ctk.VBox (0);
			vbox.homogeneous = false;
			hbox1 = new Ctk.HBox (64);
			hbox1.homogeneous = false;
			hbox1.pack (widget5, false, false);
			hbox1.pack (widget6, false, false);
			hbox1.pack (widget7, false, false);
			hbox1.pack (widget8, false, false);
			hbox2 = new Ctk.HBox (64);
			hbox2.homogeneous = false;
			hbox2.pack (widget1, false, false);
			hbox2.pack (widget2, false, false);
			hbox2.pack (widget3, false, false);
			hbox2.pack (widget4, false, false);
			vbox.pack (hbox2, false, false);
			vbox.pack (hbox1, false, false);
			add_actor (vbox);
			show_all ();
		}

		construct
		{
		}
	}
}
